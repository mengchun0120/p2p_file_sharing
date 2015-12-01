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

#include "rdfs_history.h"

int RdfsHistory::reqSize(int max)
{
	if(max <= 0 || full()) {
		return 0;
	} else if(head_ == 0) {
		return (total_ > 1 && max >= 2) ? 2 : 1;
	}
	
	int size;
	RdfsIDSegment *s;
	
	if(head_->start() == 0) {
		size = 0;
	} else {
		size = (head_->start() > 1 && max >= 2) ? 2 : 1;
	}
	
	for(s = head_; size < max && s->next() != 0; s = s->next()) {
		size += (s->next()->start() - s->end() > 2 && max - size >= 2) ? 2 : 1;
	}
	
	if(size < max && s->end() < total_ - 1) {
		size += (total_ - s->end() > 2 && max - size >= 2) ? 2 : 1;
	}
	
	return size;
}

int RdfsHistory::reqSize(int *adv, int advSize, int max)
{
	int i = 0, size = 0, rstart, start, end;
	RdfsIDSegment *s = head_, *t = 0;

	while(i < advSize && size < max) {
		if((adv[i] & SEG_MASK) == 0) {
			start = adv[i];
			end = adv[i + 1];
			i += 2;
		} else {
			start = end = (adv[i] ^ SEG_MASK);
			++i;
		}
				
		for(; s != 0 && size < max; s = s->next()) {
			if(end < s->start()) {
				rstart = (t != 0 && start <= t->end()) ? (t->end() + 1) : start;
				size += (end > rstart && max - size >= 2) ? 2 : 1;
				break;
				
			} else {
				if(start < s->start()) {
					rstart = (t != 0 && start <= t->end()) ? (t->end() + 1) : start;
					size += (s->start() - rstart > 1 && max - size >= 2) ? 2 : 1;
				}
					
				if(end <= s->end()) break;
			}
			t = s;
		}
		
		if(s == 0 && size < max) {
			rstart = (t != 0 && start <= t->end()) ? (t->end() + 1) : start;
			size += (end > rstart && max - size >= 2) ? 2 : 1;
		}
	}
	
	return size;
}

int RdfsHistory::getReq(int *req,  int rsize)
{
	if(rsize <= 0) {
		return 0;
	} else if(head_ == 0) {
		if(total_ > 1 && rsize >= 2) {
			req[0] = 0;
			req[1] = total_ - 1;
			return 2;
		} else {
			req[0] = SEG_MASK;
			return 1;
		}
	}
	
	int i = 0;
	RdfsIDSegment *s;
	
	if(head_->start() > 0) {
		if(head_->start() > 1 && rsize >= 2) {
			req[i++] = 0;
			req[i++] = head_->start() - 1;
		} else {
			req[i++] = SEG_MASK;
		}
	}
	
	for(s = head_; i < rsize && s->next() != 0; s = s->next()) {
		if(s->next()->start() - s->end() > 2 && rsize - i >= 2) {
			req[i++] = s->end() + 1;
			req[i++] = s->next()->start() - 1;
		} else {
			req[i++] = (s->end() + 1) | SEG_MASK;
		}
	}
	
	if(i < rsize && s->end() < total_ - 1) {
		if(total_ - s->end() > 2 && rsize - i >= 2) {
			req[i++] = s->end() + 1;
			req[i++] = total_ - 1;
		} else {
			req[i++] = (s->end() + 1) | SEG_MASK;
		}
	}
	
	return i;
}

int RdfsHistory::getReq(int *req, int rsize, int *adv, int advSize)
{
	int i = 0, j = 0, start, end, rstart;
	RdfsIDSegment *s = head_, *t = 0;

	while(i < advSize && j < rsize) {
		if((adv[i] & SEG_MASK) == 0) {
			start = adv[i];
			end = adv[i + 1];
			i += 2;
		} else {
			start = end = (adv[i] ^ SEG_MASK);
			++i;
		}
				
		for(; s != 0 && j < rsize; s = s->next()) {
			if(end < s->start()) {
				rstart = (t != 0 && start <= t->end()) ? (t->end() + 1) : start;
				if(end > rstart && rsize - j >= 2) {
					req[j++] = rstart;
					req[j++] = end;
				} else {
					req[j++] = rstart | SEG_MASK;
				}
				
				break;
				
			} else {
				if(start < s->start()) {
					rstart = (t != 0 && start <= t->end()) ? (t->end() + 1) : start;
					if(s->start() - rstart > 1 && rsize - j >= 2) {
						req[j++] = rstart;
						req[j++] = s->start() - 1;
					} else {
						req[j++] = rstart | SEG_MASK;
					}
				}
					
				if(end <= s->end()) break;
			}
			t = s;
		}
		
		if(s == 0 && j < rsize) {
			rstart = (t != 0 && start <= t->end()) ? (t->end() + 1) : start;
			if(end - rstart > 0 && rsize - j >= 2) {
				req[j++] = rstart;
				req[j++] = end;
			} else {
				req[j++] = rstart | SEG_MASK;
			}
		}
	}
	
	return j;
}

int RdfsHistory::advHelpfulCount(int *adv, int size, int& advCount)
{
	int i = 0, helpful = 0, ac = 0, start, end;
	RdfsIDSegment *s = head_, *t = 0;

	while(i < size) {
		if((adv[i] & SEG_MASK) == 0) {
			start = adv[i];
			end = adv[i + 1];
			i += 2;
		} else {
			start = end = (adv[i] ^ SEG_MASK);
			++i;
		}
		ac += end - start + 1;
		
		for(; s != 0; s = s->next()) {
			if(end < s->start()) {
				if(t != 0 && start <= t->end()) {
					helpful += end - t->end();
				} else {
					helpful += end - start + 1;
				}
				break;
			} else {
				if(start < s->start()) {
					if(t != 0 && start <= t->end()) {
						helpful += s->start() - t->end() - 1;
					} else {
						helpful += s->start() - start;
					}
				}
					
				if(end <= s->end()) break;
			}
			t = s;
		}
		
		if(s == 0) {
			if(t != 0 && start <= t->end()) {
				helpful += end - t->end();
			} else {
				helpful += end - start + 1;
			}
		}
	}
	
	advCount = ac;
	return helpful;	
}
