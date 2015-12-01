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
#include <cmath>
#include "random.h"
#include "mobilenode.h"
#include "rdfs_agent.h"

using namespace std;

int hdr_rdfs::offset_;
RdfsSendStatInventory RdfsAgent::sendStat_;
NodeMonitor RdfsAgent::nodeMonitor_;

char buffer[2000];

class RdfsHeaderClass : public PacketHeaderClass {
public:
	RdfsHeaderClass() : PacketHeaderClass("PacketHeader/rdfs", sizeof(hdr_rdfs)) 
	{
		bind_offset(&hdr_rdfs::offset_);	
	}
} hdr_rdfs_class;

class RdfsAgentClass : public TclClass {
public:
	RdfsAgentClass(): TclClass("Agent/RdfsAgent") {}
	TclObject *create(int argc, const char *const * argv)
	{
		assert(argc == 5);
		return new RdfsAgent(atoi(argv[4]));
	}
} rdfsagent_class;

NodeMonitor::~NodeMonitor()
{
	delete[] nodes_;
}

void NodeMonitor::init(int count)
{
	count_ = count;
	nodes_ = new MobileNode *[count_];
}

void NodeMonitor::addNode(const char *name, int idx)
{
	MobileNode *node = (MobileNode *)TclObject::lookup(name);
	if(node != 0) {
		nodes_[idx] = node;
	}
}

void NodeMonitor::getNeighbor(int idx, char *buffer)
{
	double d, dx, dy;
	int c = 0;
	buffer[0] = 0;
	for(int i = 0; i < count_; ++i) {
		if(i != idx) {
			dx = nodes_[i]->X() - nodes_[idx]->X();
			dy = nodes_[i]->Y() - nodes_[idx]->Y();
			d = sqrt(dx * dx + dy * dy);
			if(d < 250.0) {
				c += sprintf(buffer + c, "%d,", nodes_[i]->address());
			}
		}
	}
}

RdfsAgent::RdfsAgent(nsaddr_t addr):
	Agent(PT_RDFS), addr_(addr), sessions_(this)
{
	bind("broadcastDelay_", &broadcastDelay_);
	bind("broadcastJitter_", &broadcastJitter_);
	
	bind("reqDelay_", &reqDelay_);
	bind("reqCountDownDelay_", &reqCountDownDelay_);
	bind("reqHelpfulFactor_", &reqHelpfulFactor_);
	bind("advHelpfulFactor_", &advHelpfulFactor_);
	bind("reqSupReqDelay_", &reqSupReqDelay_);
	bind("reqSupDataShortDelay_", &reqSupDataShortDelay_);
	bind("reqSupDataJitter_", &reqSupDataJitter_);
	bind("reqSupClearDelay_", &reqSupClearDelay_);
	bind("reqSupConfirmDelay_", &reqSupConfirmDelay_);
	bind("maxReqSize_", &maxReqSize_);

	bind("sendTimerDelay_", &sendTimerDelay_);
	bind("maxSendCount_", &maxSendCount_);
	bind("sendWaitConfirm_", &sendWaitConfirm_);
	bind("sendWaitTimerCount_", &sendWaitTimerCount_);
	
	bind("maxAdvSize_", &maxAdvSize_);
	bind("maxDataAdvSize_", &maxDataAdvSize_);
	bind("maxSendListCount_", &maxSendListCount_);
	bind("maxConflictCount_", &maxConflictCount_);
	bind("clearDelay_", &clearDelay_);
	bind("confirmWait_", &confirmWait_);
	
	bind("checkProviderLimit_", &checkProviderLimit_);
	bind("minProviderLive_", &minProviderLive_);
	bind("realProviderLive_", &realProviderLive_);
	bind("providerCheckDelay_", &providerCheckDelay_);
	bind("reqAgentLive_", &reqAgentLive_);
}

RdfsAgent::~RdfsAgent()
{
}

int RdfsAgent::command(int argc, const char *const* argv)
{	
	Tcl& tcl = Tcl::instance();
	if(argc == 2) {
		if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", addr_);
			return (TCL_OK);
		
		}
		
	} else if(argc == 3) {
		if(strcmp(argv[1], "addr") == 0) {
			addr_ = (nsaddr_t)atoi(argv[2]);
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "recv-pkt-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->recvPktCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "join") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			tcl.resultf("%d", (fs != 0) ? 1 : 0);
			return TCL_OK;
			
		} else if(strcmp(argv[1], "recv-useless-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->uselessCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "recv-useful-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->usefulCount());
			}
			return TCL_OK;		

		} else if(strcmp(argv[1], "req-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->reqCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "confirm-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->confirmCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "clear-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->clearCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "send-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->sendCount());
			}
			return TCL_OK;
			
		}  else if(strcmp(argv[1], "cancel-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->cancelCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "send-bit-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->sendBitCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "recv-bit-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->recvBitCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "clear-bit-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->clearBitCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "req-bit-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->reqBitCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "confirm-bit-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->confirmBitCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "cancel-bit-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->cancelBitCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "pair-count") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->pairCount());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "finished") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%d", fs->src());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "has-joined") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			tcl.resultf("%d", fs != 0);
			return TCL_OK;
			
		} else if(strcmp(argv[1], "join-time") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%.5f", fs->joinTime());
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "finish-time") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				tcl.resultf("%.5f", fs->finishTime());
			}
			return TCL_OK;
		
		} else if(strcmp(argv[1], "send-useless") == 0) {
			int fileId = atoi(argv[2]);
			RdfsSendStat *ss = sendStat_.lookup(fileId);
			if(ss != 0) {
				tcl.resultf("%d", ss->uselessCount());
			}
			return TCL_OK;
		
		} else if(strcmp(argv[1], "print-send-useless") == 0) {
			int fileId = atoi(argv[2]);
			RdfsSendStat *ss = sendStat_.lookup(fileId);
			if(ss != 0) {
				for(ss->begin(); ss->cur() != 0; ss->forward()) {
					if(ss->cur()->validRecvCount() == 0) {
						ss->cur()->out(buffer);
						tcl.evalf("printdata \"%s\"", buffer);
					}
				}
			}
			return TCL_OK;
		
		} else if(strcmp(argv[1], "history") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				fs->outputHistory(buffer);
				tcl.resultf("%s", buffer);
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "send-buffer") == 0) {
			int fileId = atoi(argv[2]);
			RdfsFileSession *fs = sessions_.lookup(fileId);
			if(fs != 0) {
				fs->outputSendBuffer(buffer);
				tcl.resultf("%s", buffer);
			}
			return TCL_OK;
			
		} else if(strcmp(argv[1], "init-node-monitor") == 0) {
			int count = atoi(argv[2]);
			nodeMonitor_.init(count);
			return TCL_OK;
			
		} else if(strcmp(argv[1], "get-neighbor") == 0) {
			int idx = atoi(argv[2]);
			nodeMonitor_.getNeighbor(idx, buffer);
			tcl.resultf("%s", buffer);
			return TCL_OK;
		}
		
	} else if(argc == 4) {
		if(strcmp(argv[1], "send-recv-count") == 0) {
			int fileId = atoi(argv[2]);
			int minRecv = atoi(argv[3]);
			RdfsSendStat *ss = sendStat_.lookup(fileId);
			if(ss != 0) {
				tcl.resultf("%d", ss->validRecvCount(minRecv));
			}
			return TCL_OK;
		
		} else if(strcmp(argv[1], "log-node") == 0) {
			int idx = atoi(argv[3]);
			nodeMonitor_.addNode(argv[2], idx);
			return TCL_OK;
		}
		
	} else if(argc == 6) {
		if(strcmp(argv[1], "join-session") == 0) {
			int fileId = atoi(argv[2]);
			int fileLen = atoi(argv[3]);
			int pktLen = atoi(argv[4]);
			int src = atoi(argv[5]);
			joinSession(fileId, fileLen, pktLen, src != 0);
			return (TCL_OK);
		}
	}
	
	return Agent::command(argc, argv);
}

void RdfsAgent::recv(Packet *p, Handler *h)
{
	hdr_rdfs *rh = hdr_rdfs::access(p);
	RdfsFileSession *f = sessions_.lookup(rh->fileId_);
	if(f != 0) {
		if(rh->flag_ & hdr_rdfs::FLAG_REQ) {
			f->processReq(p);
		} else if(rh->flag_ & hdr_rdfs::FLAG_DATA) {
			f->processData(p);
		} else if(rh->flag_ & hdr_rdfs::FLAG_CONFIRM) {
			f->processConfirm(p);
		} else if(rh->flag_ & hdr_rdfs::FLAG_CLEAR) {
			f->processClear(p);
		} else if(rh->flag_ & hdr_rdfs::FLAG_CANCEL) {
			f->processCancel(p);
		} else {
			Packet::free(p);	
		}
	} else {
		Packet::free(p);
	}
}

void RdfsAgent::broadcast(Packet *p, bool waitshort)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	cmh->addr_type() = NS_AF_INET;
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->next_hop_ = IP_BROADCAST;
	cmh->num_forwards_ = 0;
	
	hdr_ip *iph = hdr_ip::access(p);
	iph->saddr() = addr_;
	iph->sport() = here_.port_;
	iph->daddr() = IP_BROADCAST;
	iph->dport() = here_.port_;
	
	double delay = (waitshort ? broadcastDelay_ / 3 : broadcastDelay_) + 
									Random::uniform() * broadcastJitter_;
	Scheduler::instance().schedule(target_, p, delay);
}

void RdfsAgent::send(Packet *p, nsaddr_t dest)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->addr_type() = NS_AF_INET;
	cmh->next_hop_ = dest;
	cmh->num_forwards_ = 0;
		
	hdr_ip *iph = hdr_ip::access(p);
	iph->saddr() = addr_;
	iph->sport() = here_.port_;
	iph->daddr() = dest;
	iph->dport() = here_.port_;
	
	target_->recv(p);
}

void RdfsAgent::joinSession(int fileId, int fileLen, int pktLen, bool src)
{
	RdfsFileSession *f = sessions_.insert(fileId, fileLen, pktLen, src);
	
	#ifdef RDFS_DEBUG
	cout << NOW_TIME << ' ' << addr_ << " join fid=" << fileId << " flen=" << fileLen
		<< " pktlen=" << pktLen << endl;
	#endif
	
	sendStat_.insert(fileId);
}
