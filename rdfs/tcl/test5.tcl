Agent/RdfsAgent set broadcastDelay_ 			0.004
Agent/RdfsAgent set broadcastJitter_			0.002

Agent/RdfsAgent set reqDelay_					6
Agent/RdfsAgent set reqCountDownDelay_			0.08
Agent/RdfsAgent set reqHelpfulFactor_			0.3
Agent/RdfsAgent set advHelpfulFactor_			0.2
Agent/RdfsAgent set reqSupReqDelay_				0.14
Agent/RdfsAgent set reqSupDataShortDelay_		0.2
Agent/RdfsAgent set reqSupDataJitter_			0.1
Agent/RdfsAgent set reqSupClearDelay_			0.06
Agent/RdfsAgent set reqSupConfirmDelay_			0.06
Agent/RdfsAgent set maxReqSize_					30

Agent/RdfsAgent set sendTimerDelay_				0.01
Agent/RdfsAgent set sendWaitConfirm_			0.08
Agent/RdfsAgent set maxSendCount_				20
Agent/RdfsAgent set sendWaitTimerCount_			5
		
Agent/RdfsAgent set maxAdvSize_					30
Agent/RdfsAgent set maxDataAdvSize_				10
Agent/RdfsAgent set maxSendListCount_			2
Agent/RdfsAgent set clearDelay_					0.15
Agent/RdfsAgent set confirmWait_				0.03
Agent/RdfsAgent set maxConflictCount_			5

Agent/RdfsAgent set checkProviderLimit_			0.0
Agent/RdfsAgent set minProviderLive_			0.3
Agent/RdfsAgent set realProviderLive_			0.08
Agent/RdfsAgent set providerCheckDelay_			0.01
Agent/RdfsAgent set reqAgentLive_				0.15

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
set val(nn)			[lindex $argv 0]
set val(speed)			[lindex $argv 1]
set val(idx)			[lindex $argv 2]
set val(rp)			RDFS
set val(x)			1000
set val(y)			1000
set val(stop)		360
set val(fid)		0				;# file id
set val(flen)		1048576			;# file length (1MB)
set val(dlen)		400				;# packet length

set ns_ [new Simulator]
set tracefd [open "../trace/test5-$val(nn)-$val(speed)-$val(idx).tr" w]
set recfd [open "../trace/test5-$val(nn)-$val(speed)-$val(idx).rec" w]
set pairfd [open "../trace/test5-$val(nn)-$val(speed)-$val(idx).pair" w]
#set nodefd [open "../trace/test5-$val(nn)-$val(speed)-$val(idx).node" w]
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
				-agentTrace OFF \
				-routerTrace OFF \
					-macTrace OFF \
				-movementTrace OFF

for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns_ node]
	set agent_($i) [new Agent/RdfsAgent [$node_($i) node-addr]]
	$ns_ attach-agent $node_($i) $agent_($i)
	$node_($i) random-motion 0
}

$agent_(0) init-node-monitor $val(nn)
for {set i 0} {$i < $val(nn)} {incr i} {
	$agent_(0) log-node $node_($i) $i
}

source "../scen/scen-n$val(nn)-s$val(speed)-x1000-y1000-t360-p0-$val(idx)"

$ns_ at 2.0 "$agent_(0) join-session $val(fid) $val(flen) $val(dlen) 1"
set t 3.0
for {set i 1} {$i < $val(nn)} {incr i} {
	$ns_ at $t "$agent_($i) join-session $val(fid) $val(flen) $val(dlen) 0"
	set t [expr $t + 1.0]
}

set t1 0.0
proc rec {} {
	global ns_ agent_ val t1 nodefd
	
	puts $nodefd "$t1 -------------------------------------------"
	for {set i 0} {$i < $val(nn)} {incr i} {
		puts $nodefd "$i: N={[$agent_(0) get-neighbor $i]} S={[$agent_($i) send-buffer $val(fid)]}"
	}
	
	set t1 [expr $t1 + 0.02]
	$ns_ at $t1 "rec"
}

proc printdata { str } {
	global recfd
	puts $recfd $str
}

set t2 3.0
set pairCount 0
set maxPairCount 0
set recTimes 0
puts $pairfd "time pair-count finished"
proc recpair {} {
	global ns_ agent_ val t2 pairCount recTimes pairfd maxPairCount
	
	set pc 0
	set srcCount 0
	for {set i 0} {$i < $val(nn)} {incr i} {
		if {[$agent_($i) has-joined $val(fid)]} {
			incr pc [$agent_($i) pair-count $val(fid)]
			if {[$agent_($i) finished $val(fid)]} {
				incr srcCount
			}
		}
	}
	
	if {$pc > $maxPairCount} {
		set maxPairCount $pc
	}
	incr pairCount $pc
	incr recTimes
	puts $pairfd "$t2 $pc $srcCount"
	
	if {$srcCount < $val(nn) && $t2 < $val(stop)} {
		set t2 [expr $t2 + 1.0]
		$ns_ at $t2 "recpair"
	}
}

proc sum {} {
	global agent_ val recfd pairCount recTimes maxPairCount
	
	set sendCount [$agent_(0) send-count $val(fid)]
	set sendBitCount [$agent_(0) send-bit-count $val(fid)]
	set reqCount [$agent_(0) req-count $val(fid)]
	set reqBitCount [$agent_(0) req-bit-count $val(fid)]
	set confirmCount [$agent_(0) confirm-count $val(fid)]
	set confirmBitCount [$agent_(0) confirm-bit-count $val(fid)]
	set clearCount [$agent_(0) clear-count $val(fid)]
	set clearBitCount [$agent_(0) clear-bit-count $val(fid)]
	set cancelCount [$agent_(0) cancel-count $val(fid)]
	set cancelBitCount [$agent_(0) cancel-bit-count $val(fid)]
	set cost [expr $sendCount + $clearCount + $confirmCount + $reqCount + $cancelCount]
	set bitCost [expr $sendBitCount + $clearBitCount + $confirmBitCount + $reqBitCount + $cancelBitCount]
	set useless [$agent_(0) recv-useless-count $val(fid)]
	set recvCount 0
	set recvBitCount 0
	set delay 0.0
	
	puts $recfd "id send send-bit req req-bit confirm confirm-bit clear clear-bit cancel cancel-bit useless recv recv-bit cost bit-cost join finish delay"
#	puts $recfd "0 s=$sendCount sb=$sendBitCount rq=$reqCount rqb=$reqBitCount cf=$confirmCount cfb=$confirmBitCount cl=$clearCount clb=$clearBitCount ca=$cancelCount cab=$cancelBitCount ul=$useless r=0 rb=0 c=$cost cb=$bitCost j=* f=* d=*"
	puts $recfd "0 $sendCount $sendBitCount $reqCount $reqBitCount $confirmCount $confirmBitCount $clearCount $clearBitCount $cancelCount $cancelBitCount $useless 0 0 $cost $bitCost * * *"
	for {set i 1} {$i < $val(nn)} {incr i} {
		set s [$agent_($i) send-count $val(fid)]
		set sb [$agent_($i) send-bit-count $val(fid)]
		set rq [$agent_($i) req-count $val(fid)]
		set rqb [$agent_($i) req-bit-count $val(fid)]
		set cf [$agent_($i) confirm-count $val(fid)]
		set cfb [$agent_($i) confirm-bit-count $val(fid)]
		set cl [$agent_($i) clear-count $val(fid)]
		set clb [$agent_($i) clear-bit-count $val(fid)]
		set ca [$agent_($i) cancel-count $val(fid)]
		set cab [$agent_($i) cancel-bit-count $val(fid)]
		set ul [$agent_($i) recv-useless-count $val(fid)]
		set r [$agent_($i) recv-useful-count $val(fid)]
		set rb [$agent_($i) recv-bit-count $val(fid)]
		set c [expr $s + $rq + $cf + $cl + $ca]
		set cb [expr $sb + $rqb + $cfb + $clb + $cab]
		set joinTime [$agent_($i) join-time $val(fid)]
		set finishTime [$agent_($i) finish-time $val(fid)]
		set d [expr $finishTime - $joinTime]
		
#		puts $recfd "$i s=$s sb=$sb rq=$rq rqb=$rqb cf=$cf cfb=$cfb cl=$cl clb=$clb ca=$ca cab=$cab ul=$ul r=$r rb=$rb c=$c cb=$cb j=$joinTime f=$finishTime d=$d"
		puts $recfd "$i $s $sb $rq $rqb $cf $cfb $cl $clb $ca $cab $ul $r $rb $c $cb $joinTime $finishTime $d"
		set sendCount [expr $sendCount + $s]
		set sendBitCount [expr $sendBitCount + $sb]
		set reqCount [expr $reqCount + $rq]
		set reqBitCount [expr $reqBitCount + $rqb]
		set confirmCount [expr $confirmCount + $cf]
		set confirmBitCount [expr $confirmBitCount + $cfb]
		set clearCount [expr $clearCount + $cl]
		set clearBitCount [expr $clearBitCount + $clb]
		set cancelCount [expr $cancelCount + $ca]
		set cancelBitCount [expr $cancelBitCount + $cab]
		set useless [expr $useless + $ul]
		set recvCount [expr $recvCount + $r]
		set recvBitCount [expr $recvBitCount + $rb]
		set cost [expr $cost + $c]
		set bitCost [expr $bitCost + $cb]
		set delay [expr $delay + $d]
	}
	set delay [expr $delay / ($val(nn) - 1)]
#	puts $recfd "sum s=$sendCount sb=$sendBitCount rq=$reqCount rqb=$reqBitCount cf=$confirmCount cfb=$confirmBitCount cl=$clearCount clb=$clearBitCount ca=$cancelCount cab=$cancelBitCount ul=$useless r=$recvCount rb=$recvBitCount c=$cost cb=$bitCost j=* f=* d=$delay"
	puts $recfd "sum $sendCount $sendBitCount $reqCount $reqBitCount $confirmCount $confirmBitCount $clearCount $clearBitCount $cancelCount $cancelBitCount $useless $recvCount $recvBitCount $cost $bitCost $delay [expr $recvBitCount.0/$bitCost.0] [expr $pairCount.0 / $recTimes.0] $maxPairCount"
#	puts $recfd "avg-delay $delay"
#	puts $recfd "efficiency [expr $recvBitCount.0/$bitCost.0]"
#	puts $recfd "avg-pair-count [expr $pairCount.0 / $recTimes.0]"
#	puts $recfd "max-pair-count $maxPairCount"
	puts $recfd "send-useless [$agent_(0) send-useless $val(fid)]"
	puts $recfd "useless pkts ---------------------------"
#	$agent_(0) print-send-useless $val(fid)
}


proc stop {} {
	global agent_ ns_ tracefd recfd val pairfd
	puts "existing"
	$ns_ flush-trace
	sum
	close $tracefd
	close $recfd
	close $pairfd
#	close $nodefd
	exit 0
}

#$ns_ at $t1 "rec"
$ns_ at $t2 "recpair"
$ns_ at $val(stop).02 "stop"
$ns_ run
