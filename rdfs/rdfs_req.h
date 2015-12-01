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

#ifndef RDFS_REQ_H
#define RDFS_REQ_H

#include "config.h"

class RdfsReq {
public:
	RdfsReq(nsaddr_t req, int reqId, double reqTime): 
		req_(req), reqId_(reqId), replied_(false), 
		reqTime_(reqTime), replyTime_(-1.0), next_(0) 
	{}
	~RdfsReq() {}
	
	nsaddr_t req() { return req_; }
	int reqId() { return reqId_; }
	bool replied() { return replied_; }
	double reqTime() { return reqTime_; }
	double replyTime() { return replyTime_; }
	void markReplied(double replyTime);
	void link(RdfsReq *n) { next_ = n; }
	RdfsReq *next() { return next_; }

protected:
	nsaddr_t req_;
	int reqId_;
	double reqTime_;
	double replyTime_;
	bool replied_;
	RdfsReq *next_;
};

class RdfsReqList {
public:
	RdfsReqList(): head_(0), count_(0) {}
	~RdfsReqList();

	int count() { return count_; }
	RdfsReq *insert(nsaddr_t req, int reqId, double reqTime);
	RdfsReq *lookup(nsaddr_t req, int reqId);
	RdfsReq *latest() { return head_; }

protected:
	RdfsReq *head_;
	int count_;
};

#endif // RDFS_REQ_H
