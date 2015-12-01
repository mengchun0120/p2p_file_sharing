/***************************************************************************
 *   Copyright (C) 2008 by mengchun   *
 *   mengchun0120@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <cstdio>
#include "rdfs_sendtimer.h"
#include "rdfs_session.h"


int RdfsSendList::retrieveFirst()
{
	if(head_ == 0) return -1;
	int id = head_->start();
	if(head_->start() + 1 <= head_->end()) {
		head_->start()++;
	} else {
		RdfsIDSegment *t = head_->next();
		delete head_;
		head_ = t;
	}
	--count_;
	return id;
}

RdfsSendWaitTimer::~RdfsSendWaitTimer()
{
	force_cancel();
	delete waitList_;
}

bool RdfsSendWaitTimer::match(nsaddr_t req, int reqId)
{
	return (waitList_ != 0) ? (waitList_->req() == req && waitList_->reqId() == reqId) : false;
}

void RdfsSendWaitTimer::setWaitList(RdfsSendList *list)
{
	if(waitList_ != 0) {
		delete waitList_;
	}
	waitList_ = list;
}

void RdfsSendWaitTimer::removeWaitList()
{
	if(waitList_ != 0) {
		delete waitList_;
		waitList_ = 0;
	}
}

RdfsSendList *RdfsSendWaitTimer::retrieveWaitList()
{
	RdfsSendList *list = waitList_;
	if(waitList_ != 0) {
		force_cancel();
		waitList_ = 0;
	}	
	return list;
}

void RdfsSendWaitTimer::expire(Event *e)
{
	session_->sendWaitTimeout(this);
}

RdfsSendWaitTimerPool::RdfsSendWaitTimerPool(RdfsFileSession *session, int count):
	count_(count)
{
	pool_ = new RdfsSendWaitTimer[count_];
	for(int i = 0; i < count_; ++i) {
		pool_[i].setSession(session);
	}
}

RdfsSendWaitTimerPool::~RdfsSendWaitTimerPool()
{
	delete[] pool_;
}

RdfsSendWaitTimer *RdfsSendWaitTimerPool::idleTimer()
{
	for(int i = 0; i < count_; ++i) {
		if(pool_[i].status() == TimerHandler::TIMER_IDLE) return &(pool_[i]);
	}
	return 0;
}

RdfsSendWaitTimer *RdfsSendWaitTimerPool::lookup(nsaddr_t req, int reqId)
{
	for(int i = 0; i < count_; ++i) {
		if(pool_[i].match(req, reqId)) return &(pool_[i]);
	}
	return 0;
}

void RdfsSendWaitTimerPool::remove(int id)
{
	RdfsSendList *list;
	for(int i = 0; i < count_; ++i) {
		if( (list = pool_[i].waitList()) != 0) {
			list->remove(id);
			if(list->count() == 0) {
				pool_[i].force_cancel();
				pool_[i].removeWaitList();
			}
		}
	}
}

void RdfsSendWaitTimerPool::cancel(nsaddr_t req)
{
	RdfsSendList *list;
	for(int i = 0; i < count_; ++i) {
		list = pool_[i].waitList();
		if(list != 0 && pool_[i].clear() == false && list->req() == req) {
			pool_[i].force_cancel();
			pool_[i].removeWaitList();
		}
	}
}

RdfsSendTimer::~RdfsSendTimer()
{
	force_cancel();
	RdfsSendList *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
}

int RdfsSendTimer::retrievePacket(nsaddr_t& req, int& reqId)
{
	if(cur_ == 0) return -1;
	
	req = cur_->req();
	reqId = cur_->reqId();
	
	int id = cur_->retrieveFirst();
	--count_;
	
	RdfsSendList *t = cur_->next();
	if(cur_->count() == 0) {
		if(prev_ != 0) {
			prev_->link(t);
		} else {
			head_ = t;
		}
		delete cur_;
		--listCount_;
		
		if(count_ == 0) {
			prev_ = cur_ = 0;
		} else if(t != 0) {
			cur_ = t;
		} else {
			cur_ = head_;
			prev_ = 0;
		}
		
	} else if(t != 0) {
		prev_ = cur_;
		cur_ = t;
		
	} else {
		cur_ = head_;
		prev_ = 0;
	}
	
	return id;
}

bool RdfsSendTimer::exist(int id)
{
	for(RdfsSendList *s = head_; s != 0; s = s->next()) {
		if(s->exist(id)) return true;
	}
	return false;
}

void RdfsSendTimer::insert(RdfsSendList *list)
{
	if(list->count() == 0) return;
	if(cur_ == 0) {
		list->link(head_);
		head_ = list;
		prev_ = 0;
		cur_ = head_;
	} else {
		list->link(cur_->next());
		cur_->link(list);
		prev_ = cur_;
		cur_ = list;
	}
	count_ += list->count();
	++listCount_;
}

void RdfsSendTimer::expire(Event *e)
{
	session_->sendTimeout();
}

int RdfsSendTimer::advSize(int max)
{
	if(max <= 0 || head_ == 0) return 0;
	
	RdfsSendList *p;
	for(p = head_; p != 0; p = p->next()) {
		p->begin();
	}
	
	int c = 0, valid = listCount_, preStart, preEnd = -1;
	RdfsSendList *q;
	RdfsIDSegment *min;
	do {
		q = head_;
		min = head_->cur();
		for(p = head_->next(); p != 0; p = p->next()) {
			if(p->cur() != 0) {
				if(min == 0 || min->start() > p->cur()->start()) {
					min = p->cur();
					q = p;
				}
			}
		}
		
		if(preEnd != -1 && preEnd + 1 == min->start()) {
			if(preStart == preEnd) ++c;
			preEnd = min->end();
		} else {
			c += (min->end() > min->start() && max - c >= 2) ? 2 : 1;
			preStart = min->start();
			preEnd = min->end();
		}
		
		q->forward();
		if(q->cur() == 0) --valid;
	} while(c < max && valid > 0);
	
	return c;
}

int RdfsSendTimer::getAdv(int *adv, int max)
{
	if(max <= 0 || head_ == 0) return 0;
	
	RdfsSendList *p, *q;
	for(p = head_; p != 0; p = p->next()) {
		p->begin();
	}
	
	int i = 0, valid = listCount_, preStart, preEnd = -1;
	RdfsIDSegment *min;
	
	do {
		min = head_->cur();
		q = head_;
		for(p = head_->next(); p != 0; p = p->next()) {
			if(p->cur() != 0) {
				if(min == 0 || min->start() > p->cur()->start()) {
					min = p->cur();
					q = p;
				}
			}
		}
		
		if(preEnd != -1 && preEnd + 1 == min->start()) {
			if(preEnd == preStart) {
				adv[i - 1] ^= SEG_MASK;
				adv[i++] = min->end();
			} else {
				adv[i - 1] = min->end();
			}
			preEnd = min->end();
			
		} else{
			if(min->start() < min->end() && max - i >= 2) {
				adv[i++] = min->start();
				adv[i++] = min->end();
			} else {
				adv[i++] = min->start() | SEG_MASK;
			}
			
			preStart = min->start();
			preEnd = min->end();
		}
		
		q->forward();
		if(q->cur() == 0) --valid;
		
	} while(i < max && valid > 0);
	
	return i;
}

void RdfsSendTimer::checkDuplicate(RdfsSendList *s)
{
	for(RdfsSendList *p = head_; p != 0 && s->count() > 0; p = p->next()) {
		s->sub(p);
	}
}

void RdfsSendTimer::output(char *buffer)
{
	int i = sprintf(buffer, "total=%d ", count_);
	for(RdfsSendList *p = head_; p != 0; p = p->next()) {
		i += p->outputStr(buffer + i);
		if(p->next() != 0) {
			i += sprintf(buffer + i, " > ");
		}
	}
}

void RdfsSendTimer::cancel(nsaddr_t addr, int *adv, int size)
{
	RdfsSegmentSet set;
	set.repopulate(adv, size);

	RdfsSendList *p, *q = 0, *t;
	int cc;
	
	for(p = head_; p != 0; p = t) {
		t = p->next();
		if(p->req() == addr) {
			cc = p->count();
			p->sub(&set);
			count_ -= cc - p->count();
			if(p->count() == 0) {
				if(q != 0) {
					q->link(t);
				} else {
					head_ = t;
				}
				delete p;
				--listCount_;
			} else {
				q = p;
			}
		} else {
			q = p;
		}
	}
	prev_ = 0;
	cur_ = head_;
	if(head_ == 0) force_cancel();
}

int RdfsSendTimer::pairCount()
{
	int pc = 0;
	
	for(RdfsSendList *p = head_; p != 0; p = p->next()) {
		RdfsSendList *q;
		for(q = head_; q != 0; q = q->next()) {
			if(q != p && q->req() == p->req()) break;
		}
		if(q == 0) ++pc;
	}
	return pc;
}