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

#ifndef RDFS_SENDSTAT_H
#define RDFS_SENDSTAT_H

#include "config.h"

class RdfsSendStatItem {
public:
	RdfsSendStatItem(nsaddr_t saddr, int uid, int pktId, 
		nsaddr_t req, int reqId, double sendTime):
		saddr_(saddr), uid_(uid), pktId_(pktId),
		req_(req), reqId_(reqId), sendTime_(sendTime), 
		validRecvCount_(0), uselessRecvCount_(0), next_(0), pre_(0)
	{}
	~RdfsSendStatItem() {}
	
	nsaddr_t saddr() { return saddr_; }
	int uid() { return uid_; }
	int pktId() { return pktId_; }
	nsaddr_t req() { return req_; }
	int reqId() { return reqId_; }
	double sendTime() { return sendTime_; }
	int& validRecvCount() { return validRecvCount_; }
	int& uselessRecvCount() { return uselessRecvCount_; }
	void linkNext(RdfsSendStatItem *next) { next_ = next; }
	void linkPre(RdfsSendStatItem *pre) { pre_ = pre; }
	RdfsSendStatItem *next() { return next_; }
	RdfsSendStatItem *pre() { return pre_; }
	void out(char *buffer);

protected:
	nsaddr_t saddr_;
	nsaddr_t req_;
	int uid_;
	int reqId_;
	int pktId_;
	double sendTime_;
	int validRecvCount_, uselessRecvCount_;
	RdfsSendStatItem *next_, *pre_;
};

class RdfsSendStat {
public:
	RdfsSendStat(int fileId): fileId_(fileId), head_(0), tail_(0), next_(0), count_(0), cur_(0) {}
	~RdfsSendStat();
	
	int fileId() { return fileId_; }
	int count() { return count_; }
	
	RdfsSendStatItem *insert(nsaddr_t saddr, int uid, int pktId, 
						nsaddr_t req, int reqId, double sendTime);
	RdfsSendStatItem *insertRev(nsaddr_t saddr, int uid, int pktId, 
						nsaddr_t req, int reqId, double sendTime);
	RdfsSendStatItem *lookup(int uid);
	RdfsSendStatItem *lookupRev(int uid);
	int uselessCount();
	int validRecvCount(int minRecvCount);
	//void printUseless();
	
	void link(RdfsSendStat *n) { next_ = n; }
	RdfsSendStat *next() {  return next_; }
	
	void begin() { cur_ = head_; }
	RdfsSendStatItem *cur() { return cur_; }
	void forward();

protected:
	int fileId_;
	RdfsSendStatItem *head_, *tail_, *cur_;
	int count_;
	RdfsSendStat *next_;
};

class RdfsSendStatInventory {
public:
	RdfsSendStatInventory(): head_(0) {}
	~RdfsSendStatInventory();
	
	RdfsSendStat* insert(int fileId);
	RdfsSendStat* lookup(int fileId);

protected:
	RdfsSendStat *head_;
};

#endif // RDFS_SENDSTAT_H
