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

#ifndef RDFS_SESSION_H
#define RDFS_SESSION_H

#include "rdfs_history.h"
#include "rdfs_sendtimer.h"
#include "rdfs_req.h"
#include "rdfs_provider.h"

class RdfsAgent;
class RdfsFileSession;

class RdfsRequestTimer: public TimerHandler {
public:
	RdfsRequestTimer(RdfsFileSession *f): f_(f), expireTime_(0) {}
	virtual ~RdfsRequestTimer() {}
	
	double expireTime() { return expireTime_; }
	void reschedReq(double delay);
	
protected:
	RdfsFileSession *f_;
	double expireTime_;
	virtual void expire(Event *e);
};

class RdfsConfirmTimer: public TimerHandler {
public:
	RdfsConfirmTimer(RdfsFileSession *f, int size);
	virtual ~RdfsConfirmTimer();
	
	void init(nsaddr_t dest, int reqId, int* a, int size);

protected:
	RdfsFileSession *f_;
	nsaddr_t dest_;
	int reqId_;
	int *adv_;
	int advSize_, advCount_;
	
	virtual void expire(Event *e);
};

class RdfsFileSession {
public:
	static void outputSeg(int *r, int count);
	
	RdfsFileSession(RdfsAgent *a, int fileId, int fileLen, int pktLen, bool src);
	~RdfsFileSession();

	int fileId() { return fileId_; }
	int fileLen() { return fileLen_; }
	int pktLen(int pktId);
	int pktCount() { return pktCount_; }
	bool src() { return src_; }
	
	int recvPktCount() { return recvPktCount_; }
	int recvBitCount() { return recvBitCount_; }
	int usefulCount() { return history_.count(); }
	int uselessCount() { return uselessCount_; }
	int reqCount() { return reqList_.count(); }
	int confirmCount() { return confirmCount_; }
	int clearCount() { return clearCount_; }
	int sendCount() { return sendCount_; }
	int cancelCount() { return cancelCount_; }
	int reqBitCount() { return reqBitCount_; }
	int sendBitCount() { return sendBitCount_; }
	int clearBitCount() { return clearBitCount_; }
	int confirmBitCount() { return confirmBitCount_; }
	int cancelBitCount() { return cancelBitCount_; }
	int pairCount() { return sendTimer_.pairCount(); }
	double joinTime() { return joinTime_; }
	double finishTime() { return finishTime_; }
	void outputHistory(char *buffer);
	void outputSendBuffer(char *buffer);

	void link(RdfsFileSession *f) { next_ = f; }
	RdfsFileSession *next() { return next_; }
	
	void reqTimeout();
	void sendWaitTimeout(RdfsSendWaitTimer *timer);
	void sendTimeout();
	void confirmTimeout(nsaddr_t dest, int reqId, int *adv, int size);
	
	void processReq(Packet *pkt);
	void processData(Packet *pkt);
	void processClear(Packet *pkt);
	void processConfirm(Packet *pkt);
	void processCancel(Packet *pkt);

protected:
	RdfsAgent *a_;
	int fileId_, fileLen_, pktLen_, pktCount_, lastPktLen_;
	bool src_;
	bool countDown_;
	RdfsSendWaitTimerPool sendWaitPool_;
	RdfsSendTimer sendTimer_;
	RdfsRequestTimer reqTimer_;
	RdfsConfirmTimer confirmTimer_;
	RdfsHistory history_;
	RdfsReqList reqList_;
	RdfsProviderList providers_;
	RdfsFileSession *next_;
	
	int recvPktCount_, uselessCount_, clearCount_;
	int confirmCount_, sendCount_, cancelCount_;
	int recvBitCount_, reqBitCount_, clearBitCount_; 
	int confirmBitCount_, sendBitCount_, cancelBitCount_;
	int reqId_;
	
	double joinTime_, finishTime_;
	
	void sendRequest(nsaddr_t dest, bool biased);
	void sendClear(RdfsSendList *list, bool biased);
	void sendConfirm(nsaddr_t dest, int reqId, int *adv, int size);
	void sendCancel(RdfsProvider *dest, int *adv, int size);
};

class RdfsFileSessionList {
public:
	RdfsFileSessionList(RdfsAgent *a);
	~RdfsFileSessionList();

	RdfsFileSession* insert(int fileId, int fileLen, int pktLen, bool src);
	RdfsFileSession* lookup(int fileId);

protected:
	RdfsAgent *a_;
	RdfsFileSession *head_;
};

#endif // RDFS_FSESSION_H
