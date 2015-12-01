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
#include "rdfs_session.h"
#include "rdfs_agent.h"
#include "random.h"

using namespace std;

void f()
{
}

void RdfsRequestTimer::reschedReq(double delay)
{
	expireTime_ = NOW_TIME + delay;
	resched(delay);
}

void RdfsRequestTimer::expire(Event *e)
{
	f_->reqTimeout();
}

RdfsConfirmTimer::RdfsConfirmTimer(RdfsFileSession *f, int size):
	f_(f), advSize_(size), advCount_(0), dest_(-1)
{
	adv_ = new int[advSize_];
}

RdfsConfirmTimer::~RdfsConfirmTimer()
{
	delete[] adv_;
}

void RdfsConfirmTimer::init(nsaddr_t dest, int reqId, int* a, int size)
{
	dest_ = dest;
	reqId_ = reqId;
	advCount_ = MIN(size, advSize_);
	for(int i = 0; i < advCount_; ++i) {
		adv_[i] = a[i];
	}
}

void RdfsConfirmTimer::expire(Event *e)
{
	f_->confirmTimeout(dest_, reqId_, adv_, advCount_);
}

void RdfsFileSession::outputSeg(int *r, int count)
{
	int i = 0;
	while(i < count) {
		if((r[i] & SEG_MASK) == 0) {
			cout << '[' << r[i] << ',' << r[i + 1] << "] ";
			i += 2;
		} else {
			cout << (r[i] ^ SEG_MASK) << ' ';
			++i;
		}
	}
}

RdfsFileSession::RdfsFileSession(RdfsAgent *a, int fileId, int fileLen, int pktLen, bool src):
	a_(a), fileId_(fileId), fileLen_(fileLen), pktLen_(pktLen), src_(src), countDown_(false),
	sendWaitPool_(this, a->sendWaitTimerCount()), sendTimer_(this),
	reqTimer_(this), confirmTimer_(this, a->maxAdvSize()),
	reqList_(), history_(), providers_(a),
	uselessCount_(0), recvPktCount_(0), clearCount_(0),
	confirmCount_(0), sendCount_(0), cancelCount_(0),
	recvBitCount_(0), reqBitCount_(0), clearBitCount_(0), confirmBitCount_(0),
	sendBitCount_(0), cancelBitCount_(0),
	joinTime_(NOW_TIME), finishTime_(-1.0),
	reqId_(0), next_(0)
{
	pktCount_ = (int)(fileLen_ / pktLen_);

	int r = fileLen_ % pktLen_;
	if(r != 0) {
		++pktCount_;
		lastPktLen_ = r;
	} else {
		lastPktLen_ = pktLen_;
	}

	history_.setTotal(pktCount_);

	if(!src_) reqTimer_.reschedReq(a_->reqDelay() / 3.0);
	providers_.resched(a_->providerCheckDelay());
}

RdfsFileSession::~RdfsFileSession()
{
}

int RdfsFileSession::pktLen(int pktId)
{
	return (pktId < pktCount_ - 1) ? pktLen_ : lastPktLen_;
}

void RdfsFileSession::outputHistory(char *buffer)
{
	if(src_) {
		sprintf(buffer, "source");
	} else {
		history_.outputStr(buffer);
	}
}

void RdfsFileSession::outputSendBuffer(char *buffer)
{
	sendTimer_.output(buffer);
}

void RdfsFileSession::reqTimeout()
{
	if(src_ || history_.full()) return;

	nsaddr_t dest;
	bool biased;
	if(providers_.count() == 0) {
		dest = UNDEFINED_TARGET;
		biased = false;
	} else {
		RdfsProvider *pd = providers_.lookupPartner();
		if(pd != 0) {
			dest = pd->addr();
			biased = false;
		} else {
			pd = providers_.latest();
			dest = pd->addr();
			biased = true;
		}
	}
	sendRequest(dest, biased);
	reqTimer_.reschedReq( a_->reqDelay() );
}

void RdfsFileSession::sendWaitTimeout(RdfsSendWaitTimer *timer)
{
	if(timer->clear() == false) {
		timer->clear() = true;
		sendClear(timer->waitList(), timer->biased());
		timer->resched(a_->sendWaitConfirm());
	} else {
		timer->removeWaitList();
	}
}

void RdfsFileSession::sendTimeout()
{
	nsaddr_t req;
	int reqId, id = sendTimer_.retrievePacket(req, reqId);
	Packet *pkt = a_->alloc();
	hdr_rdfs *rh = hdr_rdfs::access(pkt);
	hdr_cmn *cmh = hdr_cmn::access(pkt);

	rh->flag_ = hdr_rdfs::FLAG_DATA;
	rh->fileId_ = fileId_;
	rh->pktId_ = id;
	rh->req_ = req;
	rh->reqId_ = reqId;
	rh->dataLen_ = pktLen(id);
	rh->advLen_ = sendTimer_.advSize(a_->maxDataAdvSize());
	cmh->size() += IP_HDR_LEN + RDFS_HDR_LEN + rh->advLen_ * sizeof(int) + rh->dataLen_;

	int *d;
	if(rh->advLen_ > 0) {
		pkt->allocdata(rh->advLen_ * sizeof(int));
		d = (int *)pkt->accessdata();
		sendTimer_.getAdv(d, rh->advLen_);
	}

	#ifdef RDFS_DEBUG
	cout << NOW_TIME << ' ' << a_->addr() << " send " << cmh->uid() << ' ' << rh->pktId_
		<< ' ' << rh->req_ << ' ' << rh->reqId_ << " {";
	if(rh->advLen_ > 0) {
		outputSeg(d, rh->advLen_);
	}
	cout << '}' << endl;
	#endif

	RdfsSendStat *ss = RdfsAgent::sendStat().lookup(fileId_);
	ss->insertRev(a_->addr(), cmh->uid(), id, req, reqId, NOW_TIME);

	sendWaitPool_.remove(id);

	a_->broadcast(pkt);
	++sendCount_;
	sendBitCount_ += MAC802_11_HDR_LEN + cmh->size();
	if(sendTimer_.count() > 0) {
		sendTimer_.resched(a_->sendTimerDelay());
	}
}

void RdfsFileSession::confirmTimeout(nsaddr_t dest, int reqId, int *adv, int size)
{
	RdfsProvider *pd = providers_.lookup(dest);
	if(pd != 0) {
		pd->reqAgent() = true;
		pd->countDown() = false;
		sendConfirm(dest, reqId, adv, size);
	} else {
		cerr << "error: invalid minProviderLive_!" << endl;
	}
}

void RdfsFileSession::processData(Packet *pkt)
{
	++recvPktCount_;

	hdr_cmn *cmh = hdr_cmn::access(pkt);
	hdr_ip *iph = hdr_ip::access(pkt);
	hdr_rdfs *rh = hdr_rdfs::access(pkt);
	int *adv = (int *)pkt->accessdata();

	if(rh->req_ != a_->addr()) {
		sendWaitPool_.cancel(rh->req_);
	}

	RdfsSendStat *ss = RdfsAgent::sendStat().lookup(fileId_);
	RdfsSendStatItem *si = ss->lookupRev(cmh->uid());
	if(src_ || history_.exist(rh->pktId_)) {
		++uselessCount_;
		si->uselessRecvCount()++;
	} else {
		history_.insert(rh->pktId_);
		si->validRecvCount()++;
		recvBitCount_ += rh->dataLen_;

		#ifdef RDFS_DEBUG
		cout << NOW_TIME << ' ' << a_->addr() << " recv " << cmh->uid()
			<< ' ' << rh->pktId_ << ' ' << iph->saddr()
			<< ' ' << rh->req_ << ' ' << rh->reqId_ << " {";
		outputSeg(adv, rh->advLen_);
		cout << '}' << endl;
		#endif

		if(history_.count() == pktCount_) {
			reqTimer_.force_cancel();
			src_ = true;
			finishTime_ = NOW_TIME;
			providers_.force_cancel();
			#ifdef RDFS_DEBUG
			cout << NOW_TIME << ' ' << a_->addr() << " recv-finished" << endl;
			#endif
		}
	}

	if(src_) {
		Packet::free(pkt);
		return;
	}

	int help, advCount;
	help = history_.advHelpfulCount(adv, rh->advLen_, advCount);
	double factor = (advCount == 0) ? 0.0 : (double)help / (double)advCount;

	RdfsProvider *pd = providers_.lookup(iph->saddr());
	if(pd != 0) {
		pd->time() = NOW_TIME;
		pd->real() = true;
		pd->setAdv(adv, rh->advLen_);
	} else if(factor >= a_->advHelpfulFactor()) {
		pd = providers_.insert(iph->saddr(), NOW_TIME);
		pd->real() = true;
		pd->setAdv(adv, rh->advLen_);
	}

	if(pd != 0 && pd->reqAgent()) {
		if(help == 0) {		// the last packets in this cycle
			if(pd->countDown()) {
				reqTimer_.reschedReq(a_->reqSupReqDelay());
				sendRequest(pd->addr(), false);
				pd->countDown() = false;
			}
		} else if(help <= 2) {		// the last three packets in this cycle
			if(pd->countDown() == false) {
				pd->countDown() = true;
				reqTimer_.reschedReq(a_->reqCountDownDelay());
			}
		} else {
			reqTimer_.reschedReq(a_->reqSupDataShortDelay());
		}

	} else if(factor >= a_->advHelpfulFactor()) {
		confirmTimer_.force_cancel();

		if(reqTimer_.status() != TIMER_IDLE
			&& reqTimer_.expireTime() < NOW_TIME + a_->reqSupDataShortDelay()) {

			double delay = a_->reqSupDataShortDelay()
								+ Random::uniform() * a_->reqSupDataJitter();
			reqTimer_.reschedReq(delay);
		}

		pd = providers_.lookupPartner();
		if(pd != 0 && pd->conflict(adv, rh->advLen_) > a_->maxConflictCount()) {
			sendCancel(pd, adv, rh->advLen_);
			pd->reqAgent() = false;
		}
	}

	Packet::free(pkt);
}

void RdfsFileSession::processReq(Packet *pkt)
{
	hdr_cmn *cmh = hdr_cmn::access(pkt);
	hdr_ip *iph = hdr_ip::access(pkt);
	hdr_rdfs *rh = hdr_rdfs::access(pkt);
	int *req = (int *)(pkt->accessdata());
	RdfsProvider *pd = providers_.lookupPartner();
	bool supreq = (pd == 0 || pd->countDown() == false);

	sendWaitPool_.cancel(iph->saddr());		// cancel all pending send-wait-timers for the same source

	// request suppression
	if(supreq && reqTimer_.status() != TIMER_IDLE
		&& reqTimer_.expireTime() < NOW_TIME + a_->reqSupReqDelay()
		&& (rh->req_ == UNDEFINED_TARGET || providers_.lookup(rh->req_) != 0)) {

		int rc, help;
		help = history_.advHelpfulCount(req, rh->advLen_, rc);
		double r = (double)help / (double)rc;
		if(r >= a_->reqHelpfulFactor()) {
			double delay = a_->reqSupReqDelay() + Random::uniform() * a_->reqSupDataJitter();
			reqTimer_.reschedReq(delay);

			#ifdef RDFS_DEBUG
			cout << NOW_TIME << ' ' << a_->addr() << " sup-req-by-req " << cmh->uid()
				<< ' ' << iph->saddr() << ' ' << rh->req_ << ' ' << rh->reqId_
				<< ' ' << help << ' ' << rc << endl;
			#endif
		}
	}

	if(sendTimer_.listCount() == a_->maxSendListCount()) {
		Packet::free(pkt);
		return;
	}

	RdfsSendList *list;
	RdfsSendWaitTimer *timer;
	if(rh->req_ == UNDEFINED_TARGET || (rh->flag_ & hdr_rdfs::FLAG_BIASED)) {
		timer = sendWaitPool_.idleTimer();
		if(timer == 0) {
			Packet::free(pkt);
			return;
		}
		timer->biased() = (rh->flag_ & hdr_rdfs::FLAG_BIASED);
		timer->clear() = false;
		list = new RdfsSendList(iph->saddr(), rh->reqId_);

	} else if(rh->req_ == a_->addr()) {
		list = new RdfsSendList(iph->saddr(), rh->reqId_);
	} else {
		Packet::free(pkt);
		return;
	}

	list->repopulate(req, rh->advLen_);
	int pc = list->count();
	sendTimer_.checkDuplicate(list);
	pc -= list->count();

	if(pc < a_->maxSendCount()) {
		if(!src_) {
			list->common(&history_, a_->maxSendCount() - pc);
		} else {
			list->contract(a_->maxSendCount() - pc);
		}
	}

	if(pc < a_->maxSendCount() && list->count() > 0) {
		if(rh->req_ == UNDEFINED_TARGET || (rh->flag_ & hdr_rdfs::FLAG_BIASED)) {
			timer->setWaitList(list);
			double delay;
			if(rh->req_ == UNDEFINED_TARGET) {
				delay = Random::uniform() * a_->clearDelay();
			} else if(rh->req_ != a_->addr()) {
				delay = a_->reqSupReqDelay() + Random::uniform() * a_->clearDelay();
			} else {
				delay = 0;
			}
			timer->resched(delay);
		} else {
			sendTimer_.insert(list);
			if(sendTimer_.status() == TIMER_IDLE) sendTimer_.resched(0);
		}

		#ifdef RDFS_DEBUG
		cout << NOW_TIME << ' ' << a_->addr();
		if(rh->req_ == UNDEFINED_TARGET) {
			cout << " resp-wait ";
		} else {
			cout << " resp-fast ";
		}
		cout << cmh->uid() << ' ' << iph->saddr() << ' ' << rh->reqId_ << ' ';
		list->out();
		cout << endl;
		#endif
	} else {
		delete list;
	}

	Packet::free(pkt);
}

void RdfsFileSession::processClear(Packet *pkt)
{
	hdr_cmn *cmh = hdr_cmn::access(pkt);
	hdr_ip *iph = hdr_ip::access(pkt);
	hdr_rdfs *rh = hdr_rdfs::access(pkt);
	int *adv = (int *)pkt->accessdata();
	int advCount, help;
	help = history_.advHelpfulCount(adv, rh->advLen_, advCount);
	double factor = (double)help / (double)advCount;
	RdfsProvider *pd = providers_.lookupPartner();
	bool supreq = (pd == 0 || pd->countDown() == false);

	if(rh->req_ != a_->addr()) {
		sendWaitPool_.cancel(rh->req_);

		if(supreq && factor > a_->advHelpfulFactor()) {
			if(reqTimer_.status() != TIMER_IDLE
				&& reqTimer_.expireTime() < NOW_TIME + a_->reqSupClearDelay()) {

				double delay = a_->reqSupClearDelay() + Random::uniform() * a_->reqSupDataJitter();
				reqTimer_.reschedReq(delay);

				#ifdef RDFS_DEBUG
				cout << NOW_TIME << ' ' << a_->addr() << " sup-req-by-clear " << cmh->uid()
					<< ' ' << iph->saddr() << ' ' << rh->req_ << ' '
					<< rh->reqId_ << ' ' << help << ' ' << advCount << endl;
				#endif
			}

			pd = providers_.lookup(iph->saddr());
			if(pd == 0) {
				pd = providers_.insert(iph->saddr(), NOW_TIME);
			} else {
				pd->time() = NOW_TIME;
			}
			pd->setAdv(adv, rh->advLen_);
		}

	} else if(factor > a_->advHelpfulFactor()){
		// if this clear is responded to a previous request, abandon it.
		RdfsReq *req = reqList_.latest();
		if(req->reqId() == rh->reqId_ && req->replied() == false) {
			req->markReplied(NOW_TIME);

			double min = req->reqTime() - a_->checkProviderLimit();
			if(providers_.existReal(min) == false) {
				confirmTimer_.init(iph->saddr(), rh->reqId_, adv, rh->advLen_);
				if(providers_.exist(min)) {
					confirmTimer_.resched(a_->confirmWait());

					#ifdef RDFS_DEBUG
					cout << NOW_TIME << ' ' << a_->addr() << " confirm-wait " << cmh->uid()
						<< ' ' << iph->saddr() << ' ' << rh->req_ << ' '
						<< rh->reqId_ << endl;
					#endif
				} else {
					confirmTimer_.resched(0);
				}

				if(supreq && reqTimer_.expireTime() < NOW_TIME + a_->reqSupClearDelay()) {
					reqTimer_.reschedReq(a_->reqSupClearDelay());

					#ifdef RDFS_DEBUG
					cout << NOW_TIME << ' ' << a_->addr() << " sup-req-by-clear " << cmh->uid()
						<< ' ' << iph->saddr() << ' ' << rh->req_ << ' '
						<< rh->reqId_ << ' ' << help << ' ' << advCount << endl;
					#endif
				}
			}
		}

		pd = providers_.lookup(iph->saddr());
		if(pd == 0) {
			pd = providers_.insert(iph->saddr(), NOW_TIME);
		} else {
			pd->time() = NOW_TIME;
		}
		pd->setAdv(adv, rh->advLen_);
	}

	Packet::free(pkt);
}

void RdfsFileSession::processConfirm(Packet *pkt)
{
	hdr_cmn *cmh = hdr_cmn::access(pkt);
	hdr_ip *iph = hdr_ip::access(pkt);
	hdr_rdfs *rh = hdr_rdfs::access(pkt);
	int *adv = (int *)pkt->accessdata();
	int help, ac;
	RdfsProvider *pd = providers_.lookupPartner();
	bool supreq = (pd == 0 || pd->countDown() == false);

	help = history_.advHelpfulCount(adv, rh->advLen_, ac);
	if(supreq && rh->req_ != a_->addr()
		&& reqTimer_.status() != TIMER_IDLE
		&& reqTimer_.expireTime() < NOW_TIME + a_->reqSupConfirmDelay()
		&& providers_.lookup(rh->req_) != 0) {

		double factor = (double)help / (double)ac;
		if(factor >= a_->advHelpfulFactor()) {
			double delay = a_->reqSupConfirmDelay() + Random::uniform() * a_->reqSupDataJitter();
			reqTimer_.reschedReq(delay);

			#ifdef RDFS_DEBUG
			cout << NOW_TIME << ' ' << a_->addr() << " sup-req-by-confirm " << cmh->uid()
				<< ' ' << iph->saddr() << ' ' << rh->reqId_ << ' ' << help
				<< ' ' << ac << endl;
			#endif
		}
	}

	RdfsSendWaitTimer *timer = sendWaitPool_.lookup(iph->saddr(), rh->reqId_);
	if(timer != 0) {
		timer->force_cancel();
		if(rh->req_ != a_->addr()) {
			sendWaitPool_.cancel(iph->saddr());
		} else {
			RdfsSendList *list = timer->retrieveWaitList();
			if(list != 0) {
				if(list->count() > ac) list->repopulate(adv, rh->advLen_);
				sendTimer_.checkDuplicate(list);
				if(list->count() > 0) {
					sendTimer_.insert(list);
					if(sendTimer_.status() == TIMER_IDLE) sendTimer_.resched(0);
				} else {
					delete list;
				}
			}
		}
	}

	Packet::free(pkt);
}

void RdfsFileSession::processCancel(Packet *pkt)
{
	hdr_ip *iph = hdr_ip::access(pkt);
	hdr_rdfs *rh = hdr_rdfs::access(pkt);
	int *adv = (int *)pkt->accessdata();

	sendTimer_.cancel(iph->saddr(), adv, rh->advLen_);
}

void RdfsFileSession::sendRequest(nsaddr_t dest, bool biased)
{
	Packet *pkt = a_->alloc();
	hdr_cmn *cmh = hdr_cmn::access(pkt);
	hdr_rdfs *rh = hdr_rdfs::access(pkt);

	rh->flag_ = hdr_rdfs::FLAG_REQ;
	if(biased) rh->flag_ |= hdr_rdfs::FLAG_BIASED;
	rh->fileId_ = fileId_;
	rh->req_ = dest;
	rh->reqId_ = reqId_++;

	rh->advLen_ = history_.reqSize(a_->maxReqSize());
	cmh->size_ += IP_HDR_LEN + RDFS_HDR_LEN + sizeof(int) * rh->advLen_;

	pkt->allocdata( rh->advLen_ * sizeof(int) );
	int *d = (int *)pkt->accessdata();
	history_.getReq(d, rh->advLen_);

	#ifdef RDFS_DEBUG
	cout << NOW_TIME << ' ' << a_->addr();
	if(biased) {
		cout << " biased-req ";
	} else {
		cout << " req ";
	}
	cout << cmh->uid() << ' ' << rh->req_ << ' ' << rh->reqId_
		<< ' ' << providers_.count() << " {";
	outputSeg(d, rh->advLen_);
	cout << '}' << endl;
	#endif

	reqList_.insert(a_->addr(), rh->reqId_, NOW_TIME);
	reqBitCount_ += MAC802_11_HDR_LEN + cmh->size();
	a_->broadcast(pkt);
}

void RdfsFileSession::sendClear(RdfsSendList *list, bool biased)
{
	Packet *p = a_->alloc();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_rdfs *rh = hdr_rdfs::access(p);
	rh->flag_ = hdr_rdfs::FLAG_CLEAR;
	rh->req_ = list->req();
	rh->reqId_ = list->reqId();
	rh->advLen_ = list->advSize( a_->maxAdvSize() );
	cmh->size() = IP_HDR_LEN + RDFS_HDR_LEN + rh->advLen_ * sizeof(int);

	p->allocdata(rh->advLen_ * sizeof(int));
	int *d = (int *)p->accessdata();
	list->getAdv(d, rh->advLen_);

	#ifdef RDFS_DEBUG
	cout << NOW_TIME << ' ' << a_->addr();
	if(biased) {
		cout << " send-biased-clear ";
	} else {
		cout << " send-clear ";
	}
	cout << cmh->uid() << ' ' << list->req() << ' ' << list->reqId() << " {";
	outputSeg(d, rh->advLen_);
	cout << '}' << endl;
	#endif

	a_->broadcast(p);
	++clearCount_;
	clearBitCount_ += MAC802_11_HDR_LEN + cmh->size();
}

void RdfsFileSession::sendConfirm(nsaddr_t dest, int reqId, int *adv, int size)
{
	Packet *p = a_->alloc();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_rdfs *rh = hdr_rdfs::access(p);

	rh->flag_ = hdr_rdfs::FLAG_CONFIRM;
	rh->req_ = dest;
	rh->reqId_ = reqId;
	rh->advLen_ = history_.reqSize(adv, size, a_->maxReqSize());
	cmh->size() = IP_HDR_LEN + RDFS_HDR_LEN + rh->advLen_ * sizeof(int);

	p->allocdata(rh->advLen_ * sizeof(int));
	int *d = (int *)p->accessdata();
	history_.getReq(d, rh->advLen_, adv, size);

	#ifdef RDFS_DEBUG
	cout << NOW_TIME << ' ' << a_->addr() << " send-confirm " << cmh->uid()
		<< ' ' << dest << ' ' << reqId << " {";
	outputSeg(d, rh->advLen_);
	cout << '}' << endl;
	#endif

	a_->broadcast(p);
	++confirmCount_;
	confirmBitCount_ += MAC802_11_HDR_LEN + cmh->size();
}

void RdfsFileSession::sendCancel(RdfsProvider *dest, int *adv, int size)
{
	Packet *p = a_->alloc();
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_rdfs *rh = hdr_rdfs::access(p);

	rh->flag_ = hdr_rdfs::FLAG_CANCEL;
	rh->fileId_ = fileId_;
	rh->advLen_ = dest->conflictSize(adv, size, a_->maxAdvSize());
	cmh->size() = IP_HDR_LEN + RDFS_HDR_LEN + rh->advLen_ * sizeof(int);

	p->allocdata(rh->advLen_ * sizeof(int));
	int *d = (int *)p->accessdata();
	dest->getConflict(d, rh->advLen_, adv, size);

	#ifdef RDFS_DEBUG
	cout << NOW_TIME << ' ' << a_->addr() << " send-cancel " << cmh->uid()
		<< ' ' << dest->addr() << ' ';
	outputSeg(d, rh->advLen_);
	cout << endl;
	#endif

	a_->send(p, dest->addr());
	++cancelCount_;
	cancelBitCount_ += MAC802_11_HDR_LEN + cmh->size();
}

RdfsFileSessionList::RdfsFileSessionList(RdfsAgent *a):
	a_(a), head_(0)
{
}

RdfsFileSessionList::~RdfsFileSessionList()
{
	RdfsFileSession *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
}

RdfsFileSession* RdfsFileSessionList::insert(int fileId, int fileLen, int pktLen, bool src)
{
	RdfsFileSession *fsession = new RdfsFileSession(a_, fileId, fileLen, pktLen, src);
	fsession->link(head_);
	head_ = fsession;
	return fsession;
}

RdfsFileSession* RdfsFileSessionList::lookup(int fileId)
{
	RdfsFileSession *p;
	for(p = head_; p != 0 && p->fileId() != fileId; p = p->next());
	return p;
}
