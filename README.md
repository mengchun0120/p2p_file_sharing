# p2p_file_sharing

Implementation of Request Driven File Sharing Protocol (RDFS) on NS2. To compile
these source files, copy the the whole folder into the ns-2 directory, and run make.

rdfs_agent.h, rdfs_agent.cpp: The main interface of RDFS. It provides methods to
start executing RDFS, receive/send packets, etc.

rdfs_session.h, rdfs_session.cpp: Implementation of RdfsFileSession class, which
implements the core functionalities of RDFS.

rdfs_seg.h, rdfs_seg.cpp: Implementation of RdfsIDSegment class. This class is used
to record received data chunks.

rdfs_history.h, rdfs_history.cpp: Implementation of classes RdfsProvider and RdfsProviderList.
These two classes are used to keep track of potential data providers nearby a node.

rdfs_sendstat.h, rdfs_sendstat.cpp: Implementation of classes RdfsSendStatItem, RdfsSendStat,
RdfsSendStatInventory. Those classes are used to keep track of data item already sent.

rdfs_sendtimer.h, rdfs_sendtimer.cpp: Implementation of classes RdfsSendList, RdfsSendWaitTimer,
RdfsSendWaitTimerPool and RdfsSendTimer.

rdfs_req.h, rdfs_req.cpp: Implementation of classes RdfsReq, RdfsReqList. Those
classes are used to keep track of received request.
