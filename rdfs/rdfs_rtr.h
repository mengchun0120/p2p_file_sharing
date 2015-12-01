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

#ifndef RDFS_ROUTER_H
#define RDFS_ROUTER_H

#include "cmu-trace.h"
#include "priqueue.h"
#include <classifier/classifier-port.h>
#include "agent.h"

class RdfsRouterAgent: public Agent {
public:
	RdfsRouterAgent(nsaddr_t taddr);
	virtual ~RdfsRouterAgent();
	
	void recv(Packet *p, Handler *h);
	int command(int argc, const char *const* argv);
	
protected:
	PortClassifier *dmux_;
	TclObject *uptarget_;
	Trace *logtarget_;
	nsaddr_t addr_;
};

#endif // RDFS_ROUTER_H
