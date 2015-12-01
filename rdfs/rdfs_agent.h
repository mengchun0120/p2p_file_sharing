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

#ifndef RDFS_AGENT_H
#define RDFS_AGENT_H

#include "cmu-trace.h"
#include "priqueue.h"
#include <classifier/classifier-port.h>
#include "agent.h"
#include "rdfs_session.h"
#include "rdfs_sendstat.h"

class MobileNode;
class NodeMonitor {
public:
	NodeMonitor(): nodes_(0), count_(0) {}
	~NodeMonitor();

	void init(int count);
	void addNode(const char *name, int idx);
	void getNeighbor(int idx, char *buffer);

protected:
	MobileNode **nodes_;
	int count_;
};

class RdfsAgent : public Agent {
public:
	static RdfsSendStatInventory& sendStat() { return sendStat_; }
	
	RdfsAgent(nsaddr_t addr);
	virtual ~RdfsAgent();

	nsaddr_t addr() { return addr_; }
	
	double broadcastJitter() { return broadcastJitter_; }
	double broadcastDelay() { return broadcastDelay_; }
		
	double reqDelay() { return reqDelay_; }
	double reqCountDownDelay() { return reqCountDownDelay_; }
	double reqHelpfulFactor() { return reqHelpfulFactor_; }
	double advHelpfulFactor() { return advHelpfulFactor_; }
	double reqSupReqDelay() { return reqSupReqDelay_; }
	double reqSupDataShortDelay() { return reqSupDataShortDelay_; }
	double reqSupDataJitter() { return reqSupDataJitter_; }
	double reqSupClearDelay() { return reqSupClearDelay_; }
	double reqSupConfirmDelay() { return reqSupConfirmDelay_; }
	int maxReqSize() { return maxReqSize_; }
	
	double sendTimerDelay() { return sendTimerDelay_; }
	int maxSendCount() { return maxSendCount_; }
	double sendWaitConfirm() { return sendWaitConfirm_; }
	int sendWaitTimerCount() { return sendWaitTimerCount_; }
	
	int maxAdvSize() { return maxAdvSize_; }
	int maxDataAdvSize() { return maxDataAdvSize_; }
	int maxSendListCount() { return maxSendListCount_; }
	int maxConflictCount() { return maxConflictCount_; }
	double clearDelay() { return clearDelay_; }
	double confirmWait() { return confirmWait_; }
	
	double checkProviderLimit() { return checkProviderLimit_; }
	double minProviderLive() { return minProviderLive_; }
	double realProviderLive() { return realProviderLive_; }
	double providerCheckDelay() { return providerCheckDelay_; }
	double reqAgentLive() { return reqAgentLive_; }
	
	void recv(Packet *p, Handler *h);
	int command(int argc, const char *const* argv);
	
	void broadcast(Packet *p, bool waitshort = false);
	Packet *alloc() { return allocpkt(); }
	void send(Packet *p, nsaddr_t dest);
	
protected:
	static RdfsSendStatInventory sendStat_;
	static NodeMonitor nodeMonitor_;
	// protocol parameters
	double broadcastDelay_;
	double broadcastJitter_;
	
	double reqDelay_;
	double reqCountDownDelay_;
	double reqHelpfulFactor_;
	double advHelpfulFactor_;
	double reqSupReqDelay_;
	double reqSupDataShortDelay_;
	double reqSupDataJitter_;
	double reqSupClearDelay_;
	double reqSupConfirmDelay_;
	int maxReqSize_;
	
	int maxSendCount_;
	double sendWaitConfirm_;
	double sendTimerDelay_;
	int sendWaitTimerCount_;
	
	double clearDelay_;
	double confirmWait_;
	int maxAdvSize_;
	int maxDataAdvSize_;
	int maxSendListCount_;
	int maxConflictCount_;
	
	double checkProviderLimit_;
	double minProviderLive_;
	double realProviderLive_;
	double providerCheckDelay_;
	double reqAgentLive_;
	
	nsaddr_t addr_;
	RdfsFileSessionList sessions_;

	void joinSession(int fileId, int fileLen, int pktLen, bool src);
};

#endif // RDFS_AGENT_H
