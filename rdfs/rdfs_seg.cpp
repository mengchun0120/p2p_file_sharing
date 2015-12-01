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
#include <iostream>
#include "rdfs_hdr.h"
#include "rdfs_seg.h"

using namespace std;

RdfsSegmentSet::~RdfsSegmentSet()
{
	RdfsIDSegment *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
}

void RdfsSegmentSet::insert(int id)
{
	RdfsIDSegment *s, *t;
	for(s = head_, t = 0; s != 0; s = s->next()) {
		if(id >= s->start() && id <= s->end()) {
			return;
		} else if(id < s->start()) {
			break;
		}
		t = s;
	}
	
	if(t != 0 && t->end() + 1 == id) {
		t->end() = id;
		if(s != 0 && t->end() + 1 == s->start()) {
			t->end() = s->end();
			t->link(s->next());
			delete s;
		}
	} else if(s != 0 && id + 1 == s->start()) {
		s->start() = id;
	} else {
		RdfsIDSegment *v = new RdfsIDSegment(id);
		v->link(s);
		if(t != 0) {
			t->link(v);
		} else {
			head_ = v;
		}
	}
	
	++count_;
}

bool RdfsSegmentSet::exist(int id)
{
	for(RdfsIDSegment *s = head_; s != 0; s = s->next()) {
		if(id >= s->start() && id <= s->end()) {
			return true;
		} else if(id < s->start()) {
			break;
		}
	}
	return false;
}

void RdfsSegmentSet::remove(int id)
{
	RdfsIDSegment *p, *q;
	for(p = head_, q = 0; p != 0; p = p->next()) {
		if(id >= p->start() && id <= p->end()) {
			break;
		} else if(id < p->start()) {
			return;
		}
		q = p;
	}
	
	if(p == 0) return;
	
	if(id == p->start()) {
		if(p->start() + 1 <= p->end()) {
			p->start()++;
		} else {
			if(q != 0) {
				q->link(p->next());
			} else {
				head_ = p->next();
			}
			delete p;
		}
	} else if(id == p->end()) {
		p->end()--;
	} else {
		RdfsIDSegment *s = new RdfsIDSegment(id + 1, p->end());
		s->link(p->next());
		p->end() = id - 1;
		p->link(s);
	}
	
	--count_;
}

void RdfsSegmentSet::repopulate(int *adv, int size)
{
	clear();
	RdfsIDSegment *q = 0, *p;
	int i = 0;
	while(i < size) {
		if((adv[i] & SEG_MASK) == 0) {
			p = new RdfsIDSegment(adv[i], adv[i+1]);
			count_ += adv[i+1] - adv[i] + 1;
			i += 2;
		} else {
			p = new RdfsIDSegment(adv[i] ^ SEG_MASK);
			++count_;
			++i;
		}
		
		if(q != 0) {
			q->link(p);
		} else {
			head_ = p;
		}
		q = p;
	}
}

void RdfsSegmentSet::contract(int max)
{
	int rc = 0;
	RdfsIDSegment *p, *q = 0;
	for(p = head_; p != 0 && rc < max; p = p->next()) {
		rc += p->size();
		q = p;
	}
	
	if(rc >= max) {
		if(q != 0) {
			q->end() -= rc - max;
			q->link((RdfsIDSegment *)0);
			count_ = max;
		} else {
			head_ = 0;
			count_ = 0;
		}
		for(; p != 0; p = q) {
			q = p->next();
			delete p;
		}
	}
}

int RdfsSegmentSet::advSize(int max)
{
	int c = 0;
	for(RdfsIDSegment *p = head_; p != 0 && c < max; p = p->next()) {
		if(p->end() > p->start() && max - c >= 2) {
			c += 2;
		} else {
			++c;
		}
	}
	return c;
}

int RdfsSegmentSet::getAdv(int *adv, int max)
{
	int i = 0;
	for(RdfsIDSegment *p = head_; p != 0 && i < max; p = p->next()) {
		if(p->end() > p->start() && max - i >= 2) {
			adv[i++] = p->start();
			adv[i++] = p->end();
		} else {
			adv[i++] = p->start() | SEG_MASK;
		}
	}
	return i;
}

void RdfsSegmentSet::sub(RdfsSegmentSet *s)
{
	RdfsIDSegment *p = head_, *q = 0, *t = s->head(), *temp;
	
	while(p != 0 && t != 0) {
		if(p->end() < t->start()) {
			q = p;
			p = p->next();
		
		} else if(p->start() > t->end()) {
			t = t->next();
				
		} else if(p->end() <= t->end()) {
			if(p->start() < t->start()) {
				count_ -= p->end() - t->start() + 1;
				p->end() = t->start() - 1;
				q = p;
				p = p->next();
				
			} else {
				count_ -= p->size();
				temp = p->next();
				if(q != 0) {
					q->link(temp);
				} else {
					head_ = temp;
				}
				delete p;
				p = temp;
			}
		
		} else {
			if(p->start() >= t->start()) {
				count_ -= t->end() - p->start() + 1;
				p->start() = t->end() + 1;
			} else {
				count_ -= t->size();
				temp = new RdfsIDSegment(t->end() + 1, p->end());
				temp->link(p->next());
				p->link(temp);
				p->end() = t->start() - 1;
				q = p;
				p = temp;
			}
			
			t = t->next();
		}
	}
}

// when max <= 0, there is no limit
void RdfsSegmentSet::common(RdfsSegmentSet *s, int max)
{
	int rc = 0;
	RdfsIDSegment *p = head_, *q = 0, *t = s->head(), *v;
	
	while(p != 0 && t != 0 && (max <= 0 || rc < max)) {
		if(p->start() > t->end()) {
			t = t->next();
				
		} else if(p->end() < t->start()) {
			v = p->next();
			if(q != 0) {
				q->link(v);
			} else {
				head_ = v;
			}
			delete p;
			p = v;
				
		} else if(p->end() <= t->end()) {
			if(p->start() < t->start()) p->start() = t->start();
			rc += p->size();
			if(max > 0 && rc > max) {
				p->end() -= rc - max;
				rc = max;
			}
			q = p;
			p = p->next();
		
		} else {
			if(p->start() < t->start()) p->start() = t->start();
			rc += t->end() - p->start() + 1;
			if(max > 0 && rc >= max) {
				p->end() = t->end() - rc + max;
				rc = max;
				q = p;
				p = p->next();
			} else if(t->next() != 0 && t->next()->start() <= p->end()) {
				v = new RdfsIDSegment(t->end() + 1, p->end());
				v->link(p->next());
				p->link(v);
				p->end() = t->end();
				q = p;
				p = v;
			} else {
				p->end() = t->end();
				q = p;
				p = p->next();
			}
			t = t->next();
		}
	}
	
		
	if(p != 0) {
		if(q != 0) {
			q->link((RdfsIDSegment *)0);
		} else {
			head_ = 0;
		}
		for(; p != 0; p = q) {
			q = p->next();
			delete p;
		}
	}
	
	count_ = rc;
}

void RdfsSegmentSet::forward()
{
	if(cur_ != 0) cur_ = cur_->next();
}

void RdfsSegmentSet::out()
{
	RdfsIDSegment *p;
	for(p = head_; p != 0; p = p->next()) {
		cout << '[' << p->start() << ',' << p->end() << ']';
	}
}

int RdfsSegmentSet::outputStr(char *buffer)
{
	buffer[0] = (char)0;
	int i = 0;
	RdfsIDSegment *p;
	for(p = head_; p != 0; p = p->next()) {
		i += sprintf(buffer + i, "[%d,%d]", p->start(), p->end());
	}
	return i;
}

void RdfsSegmentSet::clear()
{
	RdfsIDSegment *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
	head_ = 0;
	count_ = 0;
}
