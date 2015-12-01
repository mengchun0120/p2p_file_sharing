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

#ifndef RDFS_SEG_H
#define RDFS_SEG_H

class RdfsIDSegment {
public:
	RdfsIDSegment(int start): start_(start), end_(start), next_(0) {}
	RdfsIDSegment(int start, int end): start_(start), end_(end), next_(0) {}
	~RdfsIDSegment() {}
	
	int& start() { return start_; }
	int& end() { return end_; }
	int size() { return end_ - start_ + 1; }
	void link(RdfsIDSegment *next) { next_ = next; }
	RdfsIDSegment *next() { return next_; }

protected:
	int start_, end_;
	RdfsIDSegment *next_;
};

class RdfsSegmentSet {
public:
	RdfsSegmentSet(): head_(0), count_(0) {}
	virtual ~RdfsSegmentSet();

	RdfsIDSegment *head() { return head_; }
	int count() { return count_; }

	void insert(int id);
	bool exist(int id);
	void remove(int id);
	void contract(int max);
	void repopulate(int *adv, int size);
	int advSize(int max);
	int getAdv(int *adv, int max);
	void sub(RdfsSegmentSet *s);
	void common(RdfsSegmentSet *s, int max = 0);
	void out();
	int outputStr(char *buffer);
	void clear();	
	void begin() { cur_ = head_; }
	RdfsIDSegment *cur() { return cur_; }
	void forward();

protected:
	RdfsIDSegment *head_, *cur_;
	int count_;
};

#endif //RDFS_SEG_H
