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

#ifndef RDFS_HDR_H
#define RDFS_HDR_H

#include "ip.h"
#include "packet.h"

//#define RDFS_DEBUG

struct hdr_rdfs {
	enum {
		FLAG_TEST = 0x8000,
		FLAG_DATA = 0x4000,
		FLAG_REQ = 0x2000,
		FLAG_ADV = 0x1000,
		FLAG_NO_REAL_DATA = 0x0800,
		FLAG_SRC = 0x0400,
		FLAG_CONFIRM = 0x0200,
		FLAG_CLEAR = 0x0100,
		FLAG_CANCEL = 0x0080,
		FLAG_BIASED = 0x0040
	};
	
	short fileId_;
	int pktId_;
	short flag_;
	short advLen_;
	short dataLen_;
	int reqId_;
	nsaddr_t req_;
	static int offset_;
	static hdr_rdfs *access(Packet *p) { return (hdr_rdfs*)p->access(offset_); }
};

#define RDFS_HDR_LEN		20
#define MAC802_11_HDR_LEN	58
#define NOW_TIME		(Scheduler::instance().clock())
#define MAX(x, y)		((x) >= (y) ? (x) : (y))
#define MIN(x, y)		((x) <= (y) ? (x) : (y))
#define SEG_MASK		(0x80000000)
#define UNDEFINED_TARGET	((nsaddr_t)-1)

#endif // RDFS_HDR_H
