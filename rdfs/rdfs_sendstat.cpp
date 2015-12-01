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

#include <iostream>
#include <cstdio>
#include "rdfs_sendstat.h"

using namespace std;

void RdfsSendStatItem::out(char *buffer)
{
	int i = sprintf(buffer, "%d ", uid_);
	i += sprintf(buffer + i, "%d ", pktId_);
	i += sprintf(buffer + i, "%d ", saddr_);
	i += sprintf(buffer + i, "%d ", req_);
	i += sprintf(buffer + i, "%d ", reqId_);
	sprintf(buffer + i, "%f", sendTime_);
}

RdfsSendStat::~RdfsSendStat()
{
	RdfsSendStatItem *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
}
	
RdfsSendStatItem *RdfsSendStat::insert(nsaddr_t saddr, int uid, int pktId, 
							nsaddr_t req, int reqId, double sendTime)
{
	RdfsSendStatItem *p, *q;
	for(p = head_, q = 0; p != 0; p = p->next()) {
		if(p->uid() == uid) {
			return p;
		} else if(p->uid() > uid) {
			break;
		}
		q = p;
	}
	
	RdfsSendStatItem *s = new RdfsSendStatItem(saddr, uid, pktId, req, reqId, sendTime);
	
	if(q != 0) {
		q->linkNext(s);
		s->linkPre(q);
	} else {
		head_ = s;
	}
	if(p != 0) {
		p->linkPre(s);
		s->linkNext(p);
	} else {
		tail_ = s;
	}
	++count_;
	
	return s;
}

RdfsSendStatItem *RdfsSendStat::insertRev(nsaddr_t saddr, int uid, int pktId, 
							nsaddr_t req, int reqId, double sendTime)
{
	RdfsSendStatItem *p, *q;
	for(p = tail_, q = 0; p != 0; p = p->pre()) {
		if(p->uid() == uid) {
			return p;
		} else if(p->uid() < uid) {
			break;
		}
		q = p;
	}
	
	RdfsSendStatItem *s = new RdfsSendStatItem(saddr, uid, pktId, req, reqId, sendTime);
	
	if(q != 0) {
		q->linkPre(s);
		s->linkNext(q);
	} else {
		tail_ = s;
	}
	if(p != 0) {
		p->linkNext(s);
		s->linkPre(p);
	} else {
		head_ = s;
	}
	++count_;
	
	return s;
}

RdfsSendStatItem *RdfsSendStat::lookup(int uid)
{
	RdfsSendStatItem *p;
	for(p = head_; p != 0; p = p->next()) {
		if(p->uid() == uid) {
			return p;
		} else if(p->uid() > uid) {
			break;
		}
	}
	return 0;
}

RdfsSendStatItem *RdfsSendStat::lookupRev(int uid)
{
	RdfsSendStatItem *p;
	for(p = tail_; p != 0; p = p->pre()) {
		if(p->uid() == uid) {
			return p;
		} else if(p->uid() < uid) {
			break;
		}
	}
	return 0;
}

void RdfsSendStat::forward()
{
	if(cur_ != 0) cur_ = cur_->next();
}

int RdfsSendStat::uselessCount()
{
	int c = 0;
	for(RdfsSendStatItem *p = head_; p != 0; p = p->next()) {
		if(p->validRecvCount() == 0) ++c;
	}
	return c;
}

int RdfsSendStat::validRecvCount(int minRecvCount)
{
	int c = 0;
	for(RdfsSendStatItem *p = head_; p != 0; p = p->next()) {
		if(p->validRecvCount() >= minRecvCount) ++c;
	}
	return c;
}
/*
void RdfsSendStat::printUseless()
{
	for(RdfsSendStatItem *p = head_; p != 0; p = p->next()) {
		if(p->validRecvCount() == 0) {
			cout << p->uid() << ' ' << p->pktId() << ' ' 
				<< p->saddr() << ' ' << p->req() << ' ' 
				<< p->reqId() << ' ' << p->sendTime() << endl;
		}
	}
}
*/
RdfsSendStatInventory::~RdfsSendStatInventory()
{
	RdfsSendStat *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
}
	
RdfsSendStat* RdfsSendStatInventory::insert(int fileId)
{
	RdfsSendStat *p, *q;
	for(p = head_, q = 0; p != 0; p = p->next()) {
		if(p->fileId() == fileId) {
			return p;
		} else if(p->fileId() > fileId) {
			break;
		}
		q = p;
	}
	
	RdfsSendStat *s = new RdfsSendStat(fileId);
	s->link(p);
	if(q != 0) {
		q->link(s);
	} else {
		head_ = s;
	}
	
	return s;
}

RdfsSendStat* RdfsSendStatInventory::lookup(int fileId)
{
	RdfsSendStat *p;
	for(p = head_; p != 0; p = p->next()) {
		if(p->fileId() == fileId) {
			return p;
		} else if(p->fileId() > fileId) {
			break;
		}
	}
	return 0;
}
	
