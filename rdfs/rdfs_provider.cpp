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

#include "rdfs_provider.h"
#include "rdfs_agent.h"

RdfsProvider::RdfsProvider(nsaddr_t addr, double time, int advSize): 
	addr_(addr), real_(false), reqAgent_(false), countDown_(false),
	time_(time), advSize_(advSize), next_(0) 
{
	adv_ = 	new int[advSize_];
	advCount_ = 0;
}

RdfsProvider::~RdfsProvider()
{
	delete[] adv_;
}

void RdfsProvider::setAdv(int *a, int size)
{
	advCount_ = MIN(size, advSize_);
	for(int i = 0; i < advCount_; ++i) {
		adv_[i] = a[i];
	}
}

int RdfsProvider::conflict(int *a, int size)
{
	int i = 0, j = 0, start1, end1, start2, end2, rc = 0;
	
	while(i < advCount_ && j < size) {
		if((adv_[i] & SEG_MASK) == 0) {
			start1 = adv_[i];
			end1 = adv_[i + 1];
		} else {
			start1 = end1 = adv_[i] ^ SEG_MASK;
		}
		
		if((a[j] & SEG_MASK) == 0) {
			start2 = a[j];
			end2 = a[j + 1];
		} else {
			start2 = end2 = a[j] ^ SEG_MASK;
		}
		
		if(start1 > end2) {
			j += ((a[j] & SEG_MASK) == 0) ? 2 : 1;
			
		} else if(end1 < start2) {
			i += ((adv_[i] & SEG_MASK) == 0) ? 2 : 1;
			
		} else if(end1 <= end2) {
			rc += (start1 >= start2) ? (end1 - start1 + 1) : (end1 - start2 + 1);
			i += ((adv_[i] & SEG_MASK) == 0) ? 2 : 1;
		
		} else {
			rc += (start1 >= start2) ? (end2 - start1 + 1) : (end2 - start2 + 1);
			j += ((a[j] & SEG_MASK) == 0) ? 2 : 1;
		}
	}
	
	return rc;
}

int RdfsProvider::conflictSize(int *a, int size, int max)
{
	int i = 0, j = 0, start1, end1, start2, end2, start3, sz = 0;
	
	while(i < advCount_ && j < size && sz < max) {
		if((adv_[i] & SEG_MASK) == 0) {
			start1 = adv_[i];
			end1 = adv_[i + 1];
		} else {
			start1 = end1 = adv_[i] ^ SEG_MASK;
		}
		
		if((a[j] & SEG_MASK) == 0) {
			start2 = a[j];
			end2 = a[j + 1];
		} else {
			start2 = end2 = a[j] ^ SEG_MASK;
		}
				
		if(start1 > end2) {
			j += ((a[j] & SEG_MASK) == 0) ? 2 : 1;
			
		} else if(end1 < start2) {
			i += ((adv_[i] & SEG_MASK) == 0) ? 2 : 1;
			
		} else if(end1 <= end2) {
			start3 = MAX(start1, start2);
			sz += (start3 < end1 && max - sz >= 2) ? 2 : 1;
			i += ((adv_[i] & SEG_MASK) == 0) ? 2 : 1;
		
		} else {
			start3 = MAX(start1, start2);
			sz += (start3 < end2 && max - sz >= 2) ? 2 : 1;
			j += ((a[j] & SEG_MASK) == 0) ? 2 : 1;
		}
	}
	
	return sz;
}

int RdfsProvider::getConflict(int *conf, int confSize, int *a, int size)
{
	int i = 0, j = 0, k = 0, start1, end1, start2, end2, start3;
	
	while(i < advCount_ && j < size && k < confSize) {
		if((adv_[i] & SEG_MASK) == 0) {
			start1 = adv_[i];
			end1 = adv_[i + 1];
		} else {
			start1 = end1 = adv_[i] ^ SEG_MASK;
		}
		
		if((a[j] & SEG_MASK) == 0) {
			start2 = a[j];
			end2 = a[j + 1];
		} else {
			start2 = end2 = a[j] ^ SEG_MASK;
		}
				
		if(start1 > end2) {
			j += ((a[j] & SEG_MASK) == 0) ? 2 : 1;
			
		} else if(end1 < start2) {
			i += ((adv_[i] & SEG_MASK) == 0) ? 2 : 1;
			
		} else if(end1 <= end2) {
			start3 = MAX(start1, start2);
			if(start3 < end1 && confSize - k >= 2) {
				conf[k++] = start3;
				conf[k++] = end1;
			} else {
				conf[k++] = start3 | SEG_MASK;
			}
			
			i += ((adv_[i] & SEG_MASK) == 0) ? 2 : 1;
		
		} else {
			start3 = MAX(start1, start2);
			if(start3 < end2 && confSize - k >= 2) {
				conf[k++] = start3;
				conf[k++] = end2;
			} else {
				conf[k++] = start3 | SEG_MASK;
			}
			
			j += ((a[j] & SEG_MASK) == 0) ? 2 : 1;
		}
	}
	
	return k;
}

RdfsProviderList::~RdfsProviderList()
{
	force_cancel();
	RdfsProvider *p, *q;
	for(p = head_; p != 0; p = q) {
		q = p->next();
		delete p;
	}
}

RdfsProvider *RdfsProviderList::lookupPartner()
{
	RdfsProvider *p;
	for(p = head_; p != 0 && p->reqAgent() == false; p = p->next());
	return p;
}

RdfsProvider *RdfsProviderList::lookup(nsaddr_t addr)
{
	RdfsProvider *p;
	for(p = head_; p != 0; p = p->next()) {
		if(p->addr() == addr) {
			return p;
		}
	}
	
	return 0;
}

RdfsProvider *RdfsProviderList::insert(nsaddr_t addr, double time)
{
	RdfsProvider *w = new RdfsProvider(addr, time, a_->maxSendCount());
	w->link(head_);
	head_ = w;
	++count_;
	return w;
}

void RdfsProviderList::remove(nsaddr_t addr)
{
	RdfsProvider *p, *q = 0;
	for(p = head_; p != 0 && p->addr() != addr; p = p->next()) {
		q = p;
	}
	
	if(p != 0) {
		if(q != 0) {
			q->link(p->next());
		} else {
			head_ = p->next();
		}
		delete p;
		--count_;
	}
}

bool RdfsProviderList::existReal(double minTime)
{
	RdfsProvider *p;
	for(p = head_; p != 0; p = p->next()) {
		if(p->time() >= minTime && p->real()) return true;
	}
	return false;
}

bool RdfsProviderList::exist(double minTime)
{
	RdfsProvider *p;
	for(p = head_; p != 0; p = p->next()) {
		if(p->time() >= minTime) return true;
	}
	return false;
}

void RdfsProviderList::expire(Event *e)
{
	RdfsProvider *p, *q = 0, *t;
	double delay, now = NOW_TIME;
	
	for(p = head_; p != 0; p  = t) {
		t = p->next();
		delay = now - p->time();
		if(delay > a_->minProviderLive()) {
			if(q != 0) {
				q->link(t);
			} else {
				head_ = t;
			}
			delete p;
			--count_;
		} else {
			if(p->real() && delay > a_->realProviderLive()) p->real() = false;
			if(p->reqAgent() && delay > a_->reqAgentLive()) p->reqAgent() = false;
			q = p;
		}
	}
	
	resched(a_->providerCheckDelay());
}
