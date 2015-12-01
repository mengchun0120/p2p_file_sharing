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

#include "rdfs_rtr.h"

class RdfsRouterAgentClass : public TclClass {
public:
	RdfsRouterAgentClass(): TclClass("Agent/RdfsRouter") {}
	TclObject *create(int argc, const char *const * argv)
	{
		assert(argc == 5);
		return new RdfsRouterAgent(atoi(argv[4]));
	}
} rdfsrouter_class;

RdfsRouterAgent::RdfsRouterAgent(nsaddr_t taddr):
Agent(PT_RDFSRTR)
{
	addr_ = taddr;
}

RdfsRouterAgent::~RdfsRouterAgent()
{
}
	
int RdfsRouterAgent::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();
	if(argc == 2) {
		if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", addr_);
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "uptarget") == 0) {
			if(uptarget_ != 0) {
				tcl.resultf("%s", uptarget_->name());
			}
			return (TCL_OK);
		} 
				
	} else if(argc == 3) {
		if(strcmp(argv[1], "addr") == 0) {
			addr_ = (nsaddr_t)atoi(argv[2]);
			return (TCL_OK);
		
		} else if(strcmp(argv[1], "log-target") == 0 ||
							strcmp(argv[1], "tracetarget") == 0) {
			logtarget_ = (Trace *)TclObject::lookup(argv[2]);
			if(logtarget_ == 0){
				tcl.resultf("%s cannot find object %s", argv[1], argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "uptarget") == 0) {
			uptarget_ = (TclObject *)TclObject::lookup(argv[2]);
			if(uptarget_ == 0) {
				tcl.resultf("%s: no such object %s", argv[1], argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
			
		} else if(strcmp(argv[1], "port-dmux") == 0) {
			dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
			if(dmux_ == 0) {
				tcl.resultf("%s: no such object %s", argv[1], argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);

		}
	} 
	
	return Agent::command(argc, argv);
}

void RdfsRouterAgent::recv(Packet *p, Handler *h)
{
	hdr_cmn *cmh = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	
	if(cmh->direction_ == hdr_cmn::DOWN) {
		target_->recv(p, (Handler *)0);
		
	} else if(cmh->direction_ == hdr_cmn::UP) {
		if(iph->saddr() == addr_) {
			drop(p, DROP_RTR_ROUTE_LOOP);
		} else if(iph->daddr() == IP_BROADCAST || iph->daddr() == addr_) {
			dmux_->recv(p, (Handler *)0);
		} else {
			Packet::free(p);
		}
	} else {
		Packet::free(p);
	}
}

