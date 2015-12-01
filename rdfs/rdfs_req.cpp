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

#include "rdfs_req.h"

void RdfsReq::markReplied(double replyTime)
{
	if(!replied_) {
		replied_ = true;
		replyTime_ = replyTime;
	}
}

RdfsReqList::~RdfsReqList()
{
	RdfsReq *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
}

RdfsReq *RdfsReqList::insert(nsaddr_t req, int reqId, double reqTime)
{
	RdfsReq *r = new RdfsReq(req, reqId, reqTime);
	r->link(head_);
	head_ = r;
	++count_;
	return r;
}

RdfsReq *RdfsReqList::lookup(nsaddr_t req, int reqId)
{
	RdfsReq *p;
	for(p = head_; p != 0; p = p->next()) {
		if(p->req() == req && p->reqId() == reqId) break;
	}
	
	return p;
}

