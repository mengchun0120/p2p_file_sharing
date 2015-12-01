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

#ifndef RDFS_SENDTIMER_H
#define RDFS_SENDTIMER_H

#include "timer-handler.h"
#include "rdfs_hdr.h"
#include "rdfs_seg.h"

class RdfsFileSession;
class RdfsSendTimer;

class RdfsSendList : public RdfsSegmentSet {
public:
	RdfsSendList(nsaddr_t req, int reqId): 
		RdfsSegmentSet(), req_(req), reqId_(reqId), next_(0) 
	{}
	~RdfsSendList() {}
	
	int reqId() { return reqId_; }
	nsaddr_t req() { return req_; }

	int retrieveFirst();
	void link(RdfsSendList *n) { next_ = n; }
	RdfsSendList *next() { return next_; }
	
protected:
	int reqId_;
	nsaddr_t req_;
	RdfsSendList *next_;
};

class RdfsSendWaitTimer: public TimerHandler {
public:
	RdfsSendWaitTimer(): session_(0), waitList_(0), clear_(false), biased_(false) {}
	virtual ~RdfsSendWaitTimer();
	
	bool& clear() { return clear_; }
	bool& biased() { return biased_; }
	void setSession(RdfsFileSession *session) { session_ = session; }
	bool match(nsaddr_t req, int reqId);
	void setWaitList(RdfsSendList *list);
	RdfsSendList *retrieveWaitList();
	void removeWaitList();
	RdfsSendList *waitList() { return waitList_; }

protected:
	RdfsFileSession *session_;
	RdfsSendList *waitList_;
	bool clear_, biased_;
	
	virtual void expire(Event *e);
};

class RdfsSendWaitTimerPool {
public:
	RdfsSendWaitTimerPool(RdfsFileSession *session, int count);
	~RdfsSendWaitTimerPool();
	
	RdfsSendWaitTimer *idleTimer();
	RdfsSendWaitTimer *lookup(nsaddr_t req, int reqId);
	void remove(int id);
	void cancel(nsaddr_t req);

protected:
	RdfsSendWaitTimer *pool_;
	int count_;
};

class RdfsSendTimer: public TimerHandler {
public:
	RdfsSendTimer(RdfsFileSession *session): 
		session_(session), head_(0), cur_(0), prev_(0), listCount_(0), count_(0)
	{}
	virtual ~RdfsSendTimer();
	
	int listCount() { return listCount_; }
	int count() { return count_; }
	int retrievePacket(nsaddr_t& req, int& reqId);
	bool exist(int id);
	void insert(RdfsSendList *list);
	int advSize(int max);
	int getAdv(int *adv, int max);
	int pairCount();
	void checkDuplicate(RdfsSendList *s);
	void output(char *buffer);
	void cancel(nsaddr_t addr, int *adv, int size);

protected:
	RdfsFileSession *session_;
	RdfsSendList *head_, *cur_, *prev_;
	int listCount_, count_;

	virtual void expire(Event *e);
};

#endif // RDFS_SENDTIMER_H
