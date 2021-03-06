Agent/RdfsAgent set broadcastDelay_ 			0.004
Agent/RdfsAgent set broadcastJitter_			0.002

Agent/RdfsAgent set reqDelay_					5
Agent/RdfsAgent set reqHelpfulFactor_			0.1
Agent/RdfsAgent set advHelpfulFactor_			0.2
Agent/RdfsAgent set reqSupReqDelay_				0.04
Agent/RdfsAgent set reqSupDataDelay_			0.07
Agent/RdfsAgent set reqSupDataJitter_			0.1
Agent/RdfsAgent set reqSupClearDelay_			0.1
Agent/RdfsAgent set reqSupConfirmDelay_			0.06
Agent/RdfsAgent set maxReqSize_					30

Agent/RdfsAgent set sendTimerDelay_				0.015
Agent/RdfsAgent set sendWaitConfirm_			0.03
Agent/RdfsAgent set maxSendCount_				30
Agent/RdfsAgent set sendWaitTimerCount_			2
		
Agent/RdfsAgent set maxAdvSize_					30
Agent/RdfsAgent set maxDataAdvSize_				10
Agent/RdfsAgent set maxSendListCount_			2
Agent/RdfsAgent set clearDelay_					0.06

remove-packet-header PGM PGM_SPM PGM_NAK Pushback NV LDP MPLS rtProtoLS Ping
remove-packet-header TFRC TFRC_ACK Diffusion RAP TORA IMEP MIP
remove-packet-header IPinIP Encap HttpInval MFTP SRMEXT SRM aSRM
remove-packet-header mcastCtrl CtrMcast rtProtoDV GAF Snoop TCPA TCP IVS
remove-packet-header Resv UMP Src_rt
remove-packet-header AODV SR

Phy/WirelessPhy set RXThresh_ 3.65262e-10

set val(chan)	 	Channel/WirelessChannel
set val(prop) 		Propagation/TwoRayGround
set val(netif) 		Phy/WirelessPhy
set val(mac) 		Mac/802_11
set val(ifq) 		Queue/DropTail/PriQueue
set val(ll)			LL
set val(ant)		Antenna/OmniAntenna
set val(ifqlen)		100
set val(nn)			4
set val(rp)			RDFS
set val(x)			500
set val(y)			500
set val(stop)		300
set val(fid)		0				;# file id
set val(flen)		1048576			;# file length (1MB)
set val(dlen)		400				;# packet length

set ns_ [new Simulator]
set tracefd [open "../trace/test3.tr" w]
set recfd [open "../trace/test3.rec" w]
$ns_ trace-all $tracefd

set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)

set god_ [create-god $val(nn)]

set chan_1_ [new $val(chan)]

$ns_ node-config -adhocRouting $val(rp) \
				-llType $val(ll) \
				-macType $val(mac) \
				-ifqType $val(ifq) \
				-ifqLen $val(ifqlen) \
				-antType $val(ant) \
				-propType $val(prop) \
				-phyType $val(netif) \
				-topoInstance $topo \
				-channel $chan_1_ \
				-agentTrace ON \
				-routerTrace OFF \
				-macTrace OFF \
				-movementTrace OFF

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/RdfsAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
	$node_($i) random-motion 0
}

$node_(0) set X_ 10.0
$node_(0) set Y_ 10.0
$node_(0) set Z_ 0.0
$node_(1) set X_ 100.0
$node_(1) set Y_ 10.0
$node_(1) set Z_ 0.0
$node_(2) set X_ 300.0
$node_(2) set Y_ 10.0
$node_(2) set Z_ 0.0
$node_(3) set X_ 200.0
$node_(3) set Y_ 100.0
$node_(3) set Z_ 0.0


$ns_ at 2.0 "$agent_(0) join-session $val(fid) $val(flen) $val(dlen) 1"
$ns_ at 3.0 "$agent_(1) join-session $val(fid) $val(flen) $val(dlen) 0"
$ns_ at 4.0 "$agent_(2) join-session $val(fid) $val(flen) $val(dlen) 0"
$ns_ at 5.0 "$agent_(3) join-session $val(fid) $val(flen) $val(dlen) 0"

set t 2.0
proc rec {} {
	global ns_ agent_ val recfd t
	puts $recfd "time=$t --------------------------------"
	for {set i 0} {$i < $val(nn)} {incr i} {
		if {[$agent_($i) join $val(fid)] == 1} {
			puts $recfd "*** $i ***"
			puts $recfd "history: [$agent_($i) history $val(fid)]"
			puts $recfd "send-buffer: [$agent_($i) send-buffer $val(fid)]"
		}
	}
	set t [expr $t + 0.05]
	$ns_ at $t "rec"
}

proc sum {} {
	global agent_ val
	
	set sendCount [$agent_(0) send-count $val(fid)]
	set reqCount [$agent_(0) req-count $val(fid)]
	set confirmCount [$agent_(0) confirm-count $val(fid)]
	set clearCount [$agent_(0) clear-count $val(fid)]
	set cost [expr $sendCount + $clearCount + $confirmCount]
	set useless [$agent_(0) recv-useless-count $val(fid)]
	set recvCount [$agent_(0) recv-useful-count $val(fid)]
	set delay 0.0
	
	puts "id send req confirm clear useless recv cost join finish delay"
	puts "0 $sendCount $reqCount $confirmCount $clearCount $useless $recvCount $cost * * *"
	for {set i 1} {$i < $val(nn)} {incr i} {
		set sc [$agent_($i) send-count $val(fid)]
		set rc [$agent_($i) req-count $val(fid)]
		set cc [$agent_($i) confirm-count $val(fid)]
		set clc [$agent_($i) clear-count $val(fid)]
		set uc [$agent_($i) recv-useless-count $val(fid)]
		set recv [$agent_($i) recv-useful-count $val(fid)]
		set c [expr $sc + $rc + $cc + $clc]
		set joinTime [$agent_($i) join-time $val(fid)]
		set finishTime [$agent_($i) finish-time $val(fid)]
		set d [expr $finishTime - $joinTime]
		
		puts "$i $sc $rc $cc $clc $uc $recv $c $joinTime $finishTime $d"
		set sendCount [expr $sendCount + $sc]
		set reqCount [expr $reqCount + $rc]
		set confirmCount [expr $confirmCount + $cc]
		set clearCount [expr $clearCount + $clc]
		set useless [expr $useless + $uc]
		set recvCount [expr $recvCount + $recv]
		set cost [expr $cost + $c]
		set delay [expr $delay + $d]
	}
	puts "sum $sendCount $reqCount $confirmCount $clearCount $useless $recvCount $cost * * [expr $delay / ($val(nn).0 - 1)]"
	puts "efficiency=[expr $recvCount.0/$cost.0]"
	puts "send-useless=[$agent_(0) send-useless $val(fid)]"
}


proc stop {} {
	global agent_ ns_ tracefd recfd val
	puts "existing"
	$ns_ flush-trace
	close $tracefd
	close $recfd
	sum
	puts "------------------ useless packets --------------------"
	$agent_(0) print-send-useless $val(fid)
	exit 0
}

$ns_ at $t "rec"
$ns_ at $val(stop).02 "stop"
$ns_ run
