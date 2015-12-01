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

#ifndef RDFS_PROVIDER_H
#define RDFS_PROVIDER_H

#include "config.h"
#include "timer-handler.h"

class RdfsAgent;

class RdfsProvider {
public:
	RdfsProvider(nsaddr_t addr, double time, int advSize);
	~RdfsProvider();

	nsaddr_t addr() { return addr_; }
	bool& real() { return real_; }
	bool& reqAgent() { return reqAgent_; }
	bool& countDown() { return countDown_; }
	double& time() { return time_; }
	int *adv() { return adv_; }
	int advSize() { return advSize_; }
	int advCount() { return advCount_; }
	void setAdv(int *a, int size);
	int conflict(int *a, int size);
	int conflictSize(int *a, int size, int max);
	int getConflict(int *conf, int confSize, int *a, int size);
	
	void link(RdfsProvider *n) { next_ = n; }
	RdfsProvider* next() { return next_; }

protected:
	nsaddr_t addr_;
	bool real_;
	bool reqAgent_;
	bool countDown_;
	double time_;
	int *adv_;
	int advSize_;
	int advCount_;
	RdfsProvider *next_;
	
};

class RdfsProviderList : public TimerHandler {
public:
	RdfsProviderList(RdfsAgent *a): a_(a), head_(0), count_(0) {}
	~RdfsProviderList();
	
	int count() { return count_; }
	RdfsProvider *lookupPartner();
	RdfsProvider *lookup(nsaddr_t addr);
	RdfsProvider *insert(nsaddr_t addr, double time);
	void remove(nsaddr_t addr);
	bool existReal(double minTime);
	bool exist(double minTime);
	RdfsProvider *latest() { return head_; }

protected:
	RdfsProvider *head_;
	RdfsAgent *a_;
	int count_;
	
	void expire(Event *e);
};

#endif // RDFS_PROVIDER_H
