#! /usr/bin/perl 
#
#########################################################################
# ----------------------------------------------------------------------------
#                   Postel Center for Experimental Networking
#
#                USC Information Sciences Institute (USC/ISI)
#                   Marina del Rey, California 90292, USA
#                          Copyright (c) 2002
# ----------------------------------------------------------------------------
#
# Copyrights of USC/ISI and SRI apply to this software. 
#
# Copyright (c) 2002 by the University of Southern California.
# All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation in source and binary forms for non-commercial purposes and
# without fee is hereby granted, provided that the above copyright notice
# appear in all copies and that both the copyright notice and this permission
# notice appear in supporting documentation, and that any documentation,
# advertising materials, and other materials related to such distribution and
# use acknowledge that the software was partially developed by the University 
# of Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THE UNIVERSITY OF SOUTHERN CALIFORNIA MAKES NO REPRESENTATIONS ABOUT THE
# SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.  THIS SOFTWARE IS PROVIDED "AS
# IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
# LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE.
#
#----------------------------------------------------------------------------
# Copyright (c) 1991 SRI. All rights reserved.
# 
#  Redistribution and use in source and binary forms are permitted
#  provided that the above copyright notice and this paragraph are
#  duplicated in all such forms and that any documentation,
#  advertising materials, and other materials related to such
#  distribution and use acknowledge that the software was developed
#  by SRI International, Menlo Park, CA.  The name SRI International
#  may not be used to endorse or promote products derived from this
#  software without specific prior written permission.
# 
#  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
#  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
#  WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#
########################################################################
#
#                         gengraph.pl 
#
# Author : PCEN, USC/ISI. 
# Contact: Barbara A. Denny (b_a_denny@yahoo.com)
#
# NOTE: The man page distributed as part of the documentation gives more
# details about the script. Command-line options supported by the
# script can be obtained by executing "./gengraph.pl --help" 
#
# gengraph.pl extracts the datasets selected by the user from the
# client and server side TG log files, and outputs the data in a
# format suitable for viewing by a graphing tool. The script works by
# parsing the output of dcat and in the process populating internal
# map called $stats.
#
# The script works by comparing the transmit and receive logs. It is
# possible that they are of unequal lengths. If the client side log
# has more entries than the server side log, then some packets may
# have been dropped. The server side log cannot contain more entries
# than the number transmitted when using this script. The script does
# not handle the case when multiple udp client connect to the server,
# which is the most likely reason for the server side log being longer
# than the client side log.
# 
# This scripts supports three different kinds of averages: average
# send and receive rates, average delay, and average jitter. Each is
# computed over a predefined window. The following variables are used
# to store these windows: 
# 
# Parameter                       Script Variable 
#--------------------------------------------------------------------
# average send and receive rates  $rate_computation_window 
# average delay                   $average_delay_computation_window 
# average jitter                  $average_jitter_computation_window 
#--------------------------------------------------------------------
#
#Jitter calculation. For this, we first compute the average delay over
#the entire duration of the experiment. We use this average delay
#computation to compute the deviation of the delay corresponding to
#individual data packets, i.e., jitter. The experimenter must modify
#the implementation to reflect his/her notion of jitter, e.g.,
#difference in the delay measured for successive packets. 
#
# xgraph, xplot and gnuplot are the supported graphing tools. 
#
# To change the bandwidth yardstick (eg. T1, E1 etc), modify the
# $byte_rate variable. The default is 125,000 bytes == 1,000,000 bits
# 
# In case of drops, sometimes it is helpful to show the drops on the
# negative-y axis (only drops) while plotting both the transmitted and
# dropped sequence numbers. $reflect_drops can be used to control
# this. if $reflect_drops is set to -1, then the drops will be shown
# on the negative-y axis. Setting $reflect_drops to 1.0 will the
# script generate default graphs. 
#
########################################################################

use Cwd; 
use File::Basename; 

#----- CONSTANTS ----------------------
$protocol_overhead = 40; 
$byte_rate = 125000; # equivalent to 1Mbps

$color{"SR"} = "blue"; #colors for various parameters
$color{"RR"} = "green";
$color{"DR"} = "red";
$color{"LN"} = "yellow";
$color{"SQ"} = "green";
$color{"JT"} = "orange";
$color{"IS"} = "red";
$color{"IR"} = "green";
$color{"DL"} = "blue";
$color{"AD"} = "yellow";
$color{"AJ"} = "red";


# ----- DEFAULT VALUES ----------------

$reflect_drops = -1.0; 
$rate_computation_window = 1.0;  #secs
$average_delay_computation_window = 0.5; #secs
$average_jitter_computation_window = 0.5; #secs

$options{"-c"} = "client.log"; #default client-side log file 
$options{"-s"} = "server.log"; #default server-side log file 
$options{"-f"} = "xplot";      #default format 
$options{"-o"} = "stats.xpl";  #default output filename 



# ----- MISC SETTINGS ----------------
$first_time_delay_computation = 1; 
$first_time_jitter_computation = 1; 
$Done = 0;

&find_dcat();
$dir = `basename \`pwd\``;        
chop $dir;
select((select(STDOUT), $| = 1)[$[]);   # unbuffer stdout

# ----- ACTUAL PROCESSING -------------
&processargs(); 
&open_infiles();
&get_skew();

if ($outputparameter{"DR"} == 1 && $protocol =~ /tcp/ ) {
  print "Error - Parameter \"DR\" is invalid for the protocol $protocol\n"; 
  exit; 
}
  
&get_title(); 
&process_input(); 
&compute_average_delay_and_jitter(); 
&generateoutput(); 
&close_files();


# -------- START PROCESSING -----------------
#This loop looks at the receive list and the transmit list to
#see identify if any packets have been dropped/lost. 

sub process_input { 

  $pseqno = -1;
  $prev_rcv_time = 0.0; 
  $prev_delay = -1.0; 
  
  while (&next_rcv()) {
    
    # find the transmitted pkt whose sequence number
    # matches that of the received packet. 
    
    if (eof(CLIENT)){ 
      print "Warning - $server_log has excess entries.\n"; 
      print "The current version of gengraph.pl does not correctly handle such cases. \n"; 
      last; 
    }

    for (;;) {
      
      $prev_xmt_time = $save_xmt_time;
      $_ = <CLIENT>;		# read next client line
    
      if ($_ =~ /Unix Errno/) { 
	$client_log = $options{"-c"}; 
	print "Error - $client_log contains an error. Offending line:\n$_\n"; 
	exit(1); 
      }
    
      #example input line: 
      #526.163420	Transmit 	192.168.10.1.4322	10402	576
      
      ($xtime,$xdirection,$xhost,$xseqno,$xlength) = split(' ');
      if ($xdirection eq 'Teardown' || eof(CLIENT) == 1 ) {

	if ((eof(CLIENT) eq 1) && ( $xdirection ne 'Teardown')){ 
	  print "Warning - $client_log ends unexpectedly. TG may not have shutdown cleanly\n"; 
	}

	$last_xmt_time = $prev_xmt_time;
	last; 
      }
    
      #last line looks like this 
      #30.126539	Teardown 
      next if ($xdirection ne 'Transmit');


      # note this packet sequence number and length 
      $stats{"SQ"}{$xtime} = $xseqno; 
      $stats{"LN"}{$xtime} = $xlength; 
    
      if ( $prev_xmt_time > 0.0 ) {
      
	#what to do incorrectly ordered log entries?
	#Ideally shouldnt happen. We do not handle
	#it here. 
      
	$diff = $xtime - $prev_xmt_time ;
	if ($diff < 0 ) { 
	  $client_log = $options{"-c"}; 
	  print "Error - Timestamp order in $client_log does not match sequence number order. Offending line:\n$_\n"; 
	  exit(1); 
	}
      
	#inter-arrival at the sender 
	$stats{"IS"}{$xtime} = $diff
      }
    

      #Also we cannot handle reordered entries here.     
      if ($prev_rcv_time > 0.0) {
      
	$diff = $rtime - $prev_rcv_time; 
	if ($diff < 0 ) { 
	  $server_log = $options{"-s"}; 
	  print "Error - Timestamp order in $server_log does not match sequence number order. Offending line:\n$_\n"; 
	  exit(1); 
	}
      
	#inter-arrival at the receiver 
	$temp = $rtime - $skew; 
	$stats{"IR"}{$temp} = $diff;
      }
    
      # detect any losses. We can do this only for udp. TCP hides losses
      # by retransmissions. 
      if ( $protocol =~ /udp/ ) { 

	if ( $xseqno < $rseqno ) {

	  # gap in sequence numbers => loss of packets. Deciding when
	  # exactly a packet was lost is a tricky problem.We assume,
	  # simplistically that the packet is lost half-way between the
	  # time when it was sent and the time when the first successful
	  # packet was received after that. 

	  $middle = $xtime + ($rtime - $xtime - $skew)/2;
	  $stats{"DR"}{$middle} = $xseqno; 
	}
      }
    
      $prev_rcv_time = $rtime; 
      $save_xmt_time = $xtime;
    
      #throughput computation is done over a "window" of time and uses
      #the xtime (transmit time) to identify the start of these
      #windows. check the thruput code. 
      &thruput();
    
      #we have handled all packets relevant to the packet
      #received. (transmitted + dropped). 
      last if ($xseqno >= $rseqno);
    }
  
    if ($rseqno != $xseqno) {	# out of order!!
      #code should never come here....
      printf("seqno %d OUT OF ORDER!\n",$rseqno);
      next;
    }
  
    #for the successfully received packet, compute the delay. 
    $delay = $rtime - $xtime - $skew;
    if ($delay < 0 ) {
      #but for now we look for the absolute delay
      #this might be due to rounding off errors or
      #may be the packet recording times, among others 
      #ideally this should be flagged to the user.

      $delay *= -1; 
    } 

    #note the delay and compute the average delay and delay variation. 
    $stats{"DL"}{$xtime} = $delay;		    
  
    #should add more code here if you want to compute more parameters
    #related to the input such as the average length of back-to-back
    #drops. 
    #now proceed to the next received packet
  }

  # finished with all receives. We might still have some "tail"
  # transmits. 
  printf("Done with Receives last=%d\n",$pseqno);

  #for each of the remaining transmitted packets make note of them,
  #compute the transmission rate and so on
  while (<CLIENT>) {		# drain client xmt data
  
    #look for any errors in the input 
    if ($_ =~ /Unix Errno/) { 
      $client_log = $options{"-c"}; 
      print "Error - $client_log contains an error. Offending line:\n$_\n"; 
      exit(1); 
    }

    $prev_xmt_time = $save_xmt_time;
    ($xtime, $xdirection, $xhost, $xseqno, $xlength) = split(' ');
    $save_xmt_time = $xtime;
    if ($xdirection eq 'Teardown' || eof(CLIENT) == 1 ) {      
      $last_xmt_time = $prev_xmt_time;
    }
    
    next if ($xdirection ne 'Transmit');
        
    # note this packet sequence number and length 
    $stats{"SQ"}{$xtime} = $xseqno; 
    $stats{"LN"}{$xtime} = $xlength; 
    
    if ( $prev_xmt_time > 0.0 ) {
      
      #what to do incorrectly ordered log entries?
      #Ideally shouldnt happen. We do not handle
      #it here. 
      
      $diff = $xtime - $prev_xmt_time ;
      if ($diff < 0 ) { 
	$client_log = $options{"-c"}; 
	print "Error - $client_log contains an out of order entry. Offending line:\n$_\n"; 
	exit(1); 
      }
      
      #inter-arrival at the sender 
      $stats{"IS"}{$xtime} = $diff
    }
    
    &thruput();
  
  
  }
  printf("Done with Transmits last=%d\n",$pxseqno);

  # force length change to generate a tp_per_length report
  $rlength = 1;
  $Done = 1;

  &thruput();
}



#function to compute the average delay experienced by the packets. the
# $average_delay_computation_window variable controls the window over
# which the computation is performed. 
#
sub compute_average_delay ( @_ ){

    $transmit_time = $_[0]; 
    $delay_experienced = $_[1]; 
    
    $this_period = $average_delay_computation_window; 

    if ($first_time_delay_computation){ 	
	$cum_delay = $delay_experienced; 
	$EndOfDelayWindow = $transmit_time + $this_period;  
	$first_time_delay_computation = 0; 
	$count_delayed_packets = 1;
	return; 
    }


    if ($Done) { 
	$ShortTime = $EndOfDelayWindow - $last_xmt_time; 
	$EndOfDelayWindow = $last_xmt_time; 
	$this_period = $average_delay_computation_window - $ShortTime; 
    }

    if ($transmit_time >= $EndOfDelayWindow) {
	
	#output the current value
	if ($count_delayed_packets > 0 ) { 
	    $avg = $cum_delay/$count_delayed_packets; 
	    $stats{"AD"}{$EndOfDelayWindow} = $avg; 
	}
	
	
	while ($EndOfDelayWindow < $transmit_time){ 
	    $EndOfDelayWindow += $average_delay_computation_window; 
	} 
	
	$cum_delay = $delay_experienced; 
	$count_delayed_packets = 1; 

    } else { 
	$cum_delay += $delay_experienced; 	    
	$count_delayed_packets++; 
    }
}

#function to compute the average jitter experienced by the
# packets. the $average_jitter_computation_window variable controls
# the window over which the computation is performed. This code is
# very similar to the throughput computation code as well the delay
# computation code. One of the tasks for future is to merge all of
# them into one to reduce the code size. 
#
sub compute_average_jitter ( @_ ){
    
    
    $transmit_time = $_[0]; 
    $jitter_experienced = $_[1]; 
    $this_period = $average_jitter_computation_window; 

    if ($first_time_jitter_computation){ 	
	$EndOfJitterWindow = $transmit_time + $this_period; 
	$first_time_jitter_computation = 0;
	$cum_jitter = $jitter_experienced; 
	$count_jitter_packets = 1; 
	return; 
    }

    if ($Done) { 
	$ShortTime = $EndOfJitterWindow - $last_xmt_time; 
	$EndOfJitterWindow = $last_xmt_time; 
	$this_period = $average_jitter_computation_window - $ShortTime; 
    }

    if ($transmit_time >= $EndOfJitterWindow) {

	if ($count_jitter_packets > 0 ) { 
	    $avg = $cum_jitter/$count_jitter_packets; 
	    $stats{"AJ"}{$EndOfJitterWindow} = $avg; 
	}
	
	#move the window forward...... What if the next transmit time
	#happens to occur sometime into the future ? 
	while ( $EndOfJitterWindow < $transmit_time ){ 
	    $EndOfJitterWindow += $average_jitter_computation_window;  
	}
	$cum_jitter = $jitter_experienced; 
	$count_jitter_packets = 1; 

    } else { 
	$cum_jitter += $jitter_experienced; 	  
	$count_jitter_packets++; 
    }
    
}

sub compute_average_delay_and_jitter { 
  
  @times = sort {$a <=> $b} keys %{$stats{"DL"}}; 
  $max = $#times;  
  $cum_delay = 0.0;  

  $Done = 0; 
  for ($index = 0; $index <= $max; $index++){
    $instance = $times[$index];
    $delay = $stats{"DL"}{$instance}; 
    compute_average_delay($instance, $value); 
    $cum_delay += $delay; 
  }
  $Done = 1; 
  $instance = $times[$max];
  $delay = $stats{"DL"}{$instance}; 
  compute_average_delay($instance, $value); 
  $cum_delay += $delay; 
  
  $average_delay = $cum_delay/($max + 1); 

  $max = $#times;
  $Done = 0; 
  for ($index = 0; $index < $max; $index++){
    $instance = $times[$index];
    $delay = $stats{"DL"}{$instance}; 
    $jitter = $delay - $average_delay; 
    $stats{"JT"}{$instance} = $jitter;
    compute_average_jitter($instance, $jitter); 
  }
  $Done = 1; 
  $instance = $times[$max];
  $delay = $stats{"DL"}{$instance}; 
  $jitter = $delay - $average_delay; 
  $stats{"JT"}{$instance} = $jitter;
  compute_average_jitter($instance, $jitter); 
  
  
}

#this routine should do more. Currently it can take care of only two
#entries. Multihop network might actually have more reordering. Future
#work. 

sub next_rcv {	# does local 2 line sort on seqno

  if ($next_rcv_init == 0) { # first time read an extra line
    $next_rcv_init++;
    &next_rcv_line();
  }
  
  if ($save_seqno ne 0 ) { 
    $rtime = $stime;
    $rlength = $slength;
    $rseqno = $sseqno;    
    $save_seqno = 0; 
  }

  # read next line and test for end of file
  if(! &next_rcv_line()) {	

    if ($save_seqno) { # end of file, saved one left
      $rtime = $stime;
      $rlength = $slength;
      $rseqno = $save_seqno;
      $save_seqno = 0;
      return 1;
    }
    else {	# file and saved line exhausted
      return 0;
    }
  }
  
  if ($pseqno >= $sseqno) {
    # this can be used to detect reordering should the user be
    # interested in it. printf("\nseqno %d precedes
    # %d\n",$pseqno,$sseqno); printf(BADSEQ "seqno %d preceeds
    # %d\n",$pseqno,$sseqno);
  }
  $pseqno = $sseqno;
  
  # now have two pkts to choose from, use the one with lower seqno
  if ($sseqno < $rseqno) {
    $temp = $rtime;
    $rtime = $stime;
    $stime = $temp;
    
    $temp = $rlength;
    $rlength = $slength;
    $slength = $temp;

    $temp = $rseqno;
    $rseqno = $sseqno;
    $sseqno = $temp;
  }
  $save_seqno = $sseqno;
  
  return 1;
}

sub next_rcv_line {
  
  return 0 if $rcv_exhausted;	       
  while (<SERVER>) {
    ($stime, $sdirection,$shost ,$sseqno, $slength) = split(' ');
    return 1 if ($sdirection eq 'Receive');
  }
  $rcv_exhausted = 1;
  return 0;
}

# a subset of the data sets selected using the arguments are
# "expanded" internally (just the char sequence representing the
# argument) to make it easier for lookups in the output functions. 

sub processargs {

  $maxargs = $#ARGV;
  $data = 0;
  for ($index = 0; $index <= $maxargs; $index++){
    my $arg = $ARGV[$index];
    my $val = $ARGV[$index + 1];

    if ($arg =~ /(-c|-s|-o|-f)/ ){
      $options{$arg} = $val;
      $index++;
      next;
    }

    if ( $arg =~ /(-h|--help)/ ){
      print "gengraph.pl takes the output of dcat and converts into output\n" ;
      print "readable by xgraph, xplot or gnuplot\n"; 
      print "Right now supports only throughput and sequence numbers\n"; 
      printf("Usage: %s [-h|--help] [-c <client log>] [-s <server log>]\n",$0);
      printf("[-o outputfile] [-f xplot|xgraph|gnuplot ] [-SQ] [-DR]\n");
      printf("[-D] [-S] [-R] [-IS] [-IR] [-J] [-L] [-AD] [-AJ] \n");
      printf("-SQ : Plot of Packet Sequence Numbers\n"); 
      printf("-DR : Plot of the sequence numbers of the dropped packets\n"); 
      printf("-R : Plot of the Receive Rate\n"); 
      printf("-S : Plot of the Send Rate\n"); 
      printf("-IS : Plot of Inter-departure times at the Sender\n"); 
      printf("-IR : Plot of Inter-arrival times at the Receiver\n"); 
      printf("-D : Plot of End-to-end Delay\n"); 
      printf("-AD : Plot of Average End-to-end Delay\n"); 
      printf("-J : Plot of End-to-end Jitter\n"); 
      printf("-AJ : Plot of Average Jitter\n"); 
      printf("-L : Plot of Packet Lengths\n"); 
      printf("-h | --help : Print this list options\n"); 
      exit(0);  
    }

    if ( $arg =~ /(-SQ|-S|-R|-L|-IS|-IR|-DR|-D|-J|-AD|-AJ)/ ){
      $data = 1; 
      $arg =~ s/-//g; 
      
      if ($arg =~ /(SQ|IS|IR|DR|AD|AJ)/ ){ 
	  $outputparameter{$arg} = 1; 
	  next; 
      }
      
      if ($arg eq "S")
             { $outputparameter{"SR"} = 1;  next; }
      if ($arg eq "R")
             { $outputparameter{"RR"} = 1;  next; }
      if ($arg eq "L")
             { $outputparameter{"LN"} = 1;  next; }
      if ($arg eq "J")
             { $outputparameter{"JT"} = 1;  next; }
      if ($arg eq "D")
             { $outputparameter{"DL"} = 1;  next; }
      
    }
    
    
    #fall through for arguments that dont match any of the above. 

    printf("Usage: %s [-h|--help] [-c <client log>] [-s <server log>]\n",$0);
    printf("[-o outputfile] [-f xplot|xgraph|gnuplot ] [-SQ] [-DR]\n");
    printf("[-D] [-S] [-R] [-IS] [-IR] [-J] [-L] [-AD] [-AJ] \n");
    printf("-SQ : Plot of Packet Sequence Numbers\n"); 
    printf("-DR : Plot of the sequence numbers of the dropped packets\n"); 
    printf("-R : Plot of the Receive Rate\n"); 
    printf("-S : Plot of the Send Rate\n"); 
    printf("-IS : Plot of Inter-departure times at the Sender\n"); 
    printf("-IR : Plot of Inter-arrival times at the Receiver\n"); 
    printf("-D : Plot of End-to-end Delay\n"); 
    printf("-AD : Plot of Average End-to-end Delay\n"); 
    printf("-J : Plot of End-to-end Jitter\n"); 
    printf("-AJ : Plot of Average Jitter\n"); 
    printf("-L : Plot of Packet Lengths\n"); 
    printf("-h | --help : Print this list options\n"); 
    exit(1);
  }
  
  if ($data == 0 ){
    # no output format has been specified. generate sequence numbers  
      $outputparameter{"SQ"} = 1; 
  }

  if ($rate_computation_window <= 0.0){ 
      print "Error. \$rate_computation_window cannot be less than 0\n"; 
      exit(1); 
  }
  if ($average_delay_computation_window <= 0.0 ) {
      print "Error. \$delay_computation_window cannot be less than 0\n"; 
      exit(1); 
  }
  if ($average_jitter_computation_window <= 0.0 ) {
      print "Error. \$delay_computation_window cannot be less than 0\n"; 
      exit(1); 
  }
  if ($reflect_drops ne -1.0 && $reflect_drops ne 1.0){ 
      print "Error. \$reflect_drops must be set to 1.0 or -1.0 \n"; 
      exit(1); 
  }
    
  $outputformat = $options{"-f"}; 
  

}


sub close_files {
	close(SERVER);
	close(CLIENT);
}

# compute/print thruput per length, sec and offerrate per sec the
# function looks at the timestamp and decides whether the window has
# been "exceeded" or not. If yes, then an average rate is computed
# over the window. 

sub thruput {	

  #length of time period for each thruput value
  $period = $rate_computation_window; 
  
  if ($endOfSec == 0) {	# initial call
    $endOfSec = $xtime + $period;
    $sec = 0;
    $prseqno = -1;    
  }
  else {
    $this_period = $period;
    if($Done) {
      #printf("  Last xtime = $last_xmt_time, End of Section = $endOfSec \n");  
      $ShortTime = ($endOfSec - $last_xmt_time);
      $endOfSec = $last_xmt_time;
      $this_period = $period - $ShortTime;
      printf("  Last Period Short by %2.4f seconds\n",$ShortTime);
      printf("\n");
    }
    if($xtime >= $endOfSec) {
      $sec++;			
      $rcvLastSec = $RcvBytesThisSec/($byte_rate * $this_period);
      $xmtLastSec = $XmtBytesThisSec/($byte_rate * $this_period);
      if ($endOfSec > 0 ) { 
	$stats{"SR"}{$endOfSec} = $xmtLastSec; 
	$stats{"RR"}{$endOfSec} = $rcvLastSec;
      }
      
      $endOfSec += $period;
      
      $RcvBytesThisSec = 0;				
      $XmtBytesThisSec = 0;				
    }
  }
  $XmtBytesThisSec += $xlength + $protocol_overhead;
  if ( $protocol =~ /udp/ ) { 
    #space corresponding to the packet sequence number
    $XmtBytesThisSec += 4; 
  }
  $pxseqno = $xseqno;
  
  return if ($rseqno == $prseqno);
  
  # rcvd a pkt at server!
  $prseqno = $rseqno;
  if ( $protocol =~ /udp/ ) { 
    #space corresponding to the packet sequence number
    $RcvBytesThisSec += 4; 
  }
  $RcvBytesThisSec += $rlength + $protocol_overhead;  
}

sub open_infiles {
# We will need to look at the extension of the filename to
# figure out the correct command to run. gzip -d -c or zcat or plain
# cat. for now we use plain cat.         
  $client_file = $options{"-c"}; 
  $server_file = $options{"-s"}; 
  
  
  if (! -e $client_file ){ 
    print "Error - Can't open input file $client_file \n"; 
    exit(1); 
  }
  if (! -e $server_file ){ 
    print "Error - Can't open input file $server_file \n"; 
    exit(1); 
  }

  $command = "$dcat $client_file | ";
  open(CLIENT,$command) || die "Can't open $client_file: $!\n"; 
  $command = "$dcat $server_file |";
  open(SERVER,$command) || die "Can't open $server_file: $!\n";

}


sub find_dcat {


  $fullpath = $0; 
  
  $fulldir = dirname($fullpath); 
  $platform = `uname -s` ;
  $arch = `uname -m` ;
  
  chomp($arch); 
  chomp($platform); 
  
  if ($platform =~ /Linux/) {
    $subdir = "linux"; 
  } elsif ($platform =~ /FreeBSD/) {
    $subdir = "freebsd"; 
  } elsif ($platform =~ /SunOS/) {
    if ($arch =~ /sun4m/) {
      $subdir = "sunos"; 
    } else {
      $subdir = "solaris"; 
    }    
  } else {
    $subdir = $platform; 
  }
  
  $correctpath = "$fulldir/$subdir/dcat"; 
  #print "correct path = $correctpath\n"; 
  if (-x $correctpath){ 
    $dcat = $correctpath; 
  } else {
    print "Error - Did not find dcat in directory $correctpath \n"; 
    exit(1); 
  }
}

#Pick the first line from the readme if it exists. A simple way to
#uniquely identify the experiment and capture it in the graphs
#generated. 

sub get_title {
  # is there a readme file with a Title?
  
  if (-e "readme") {
    $readme_title = `head -1 readme`;
    $readme_title_exists = 1; 
    chop($readme_title);
  } elsif ( -e "Readme") {
    $readme_title = `head -1 Readme`;
    chop($readme_title);
    $readme_title_exists = 1; 
  }  elsif ( -e "README") {
    $title = `head -1 README`;
    chop($readme_title);
    $readme_title_exists = 1; 
  }    
}


#compute the skew between the two end point and also extract the date
#from the header of the client log. 
sub get_skew {

  # find client/server log clock skew
  for (;;) {
    $_ = <SERVER>;
    ($f1,$f2,$f3,$f4,$f5,$f6,$f7) = split(' ');
    last if ($f6 eq 'epoch');
  }
  $skew = $f7;
  for (;;) {
    $_ = <CLIENT>;
    ($f1,$f2,$f3,$f4,$f5,$f6,$f7) = split(' ');
    last if ($f6 eq 'epoch');
  }
  $skew = $f7 - $skew;
  #printf("skew=%d\n",$skew);
  
  # now look for "Program start time: Fri Nov 15 10:21:05 1991"
  for (;;) {
    $_ = <CLIENT>;
    ($f1,$f2,$f3,$f4,$f5,$f6,$f7) = split(' ');
    last if ($f1 eq 'Program');	# must be next line
  }
  s/Program start time://;	# chop off front
  $date = $_;
  chop $date;
  
  # now look for "Protocol under test.." 
  for (;;) {
    $_ = <CLIENT>;
    ($f1,$f2,$f3,$f4,$f5,$f6,$f7) = split(' ');
    last if ($f1 eq 'Protocol');	# must be next line
  }
  s/Protocol under test is//;	# chop off front
  $protocol = $_;
  chop $protocol;

  return;
  
  # skew based on setup is potentially more accurate than epochs
  #because it is not rounded off to the nearest sec. find
  #client/server log clock skew (based on 'Setup') 
  #for (;;) { 
  #  $_ =  <SERVER>; 
  #  ($f1,$f2) = split(' '); 
  #  last if ($f2 eq 'Setup'); 
  #} $skew = $f1; 
  #for (;;) { 
  #  $_ = <CLIENT>; 
  #  ($f1,$f2) = split(' '); 
  #  last if ($f2 eq 'Setup'); 
  #} 
  #$skew = $f1 - $skew; 
  #printf("skew=%d\n",$skew);
  
}

############################################################################
# Data generation portion is complete and output the data sets
# selected. 
#
# 1. Data is available as $stats{<parameter>}{<timestamp>}. The script
# obtains the keys (set of timestamps) for a given parameter and
# executes a loop printing all the data. 
#
# 2. options from the user are available in the $options{<option>} map
#
# 3. output format is available as $outputformat 
#
# 4. The colors for individual graphs are set in the $color map at the
# beginning of the script. 
#
#

# output datasets in xgraph formats. 
#

# xgraphs' input is just 
# "<parameter 1>"
# x1 y1 
# x2 y2
# ... 
# "<parameter 2>"
# x1 y1 
# x2 y2
# ... 
# The command line options actually control how this is displayed.

sub generate_xgraph_output() {

  $output_file = $options{"-o"}; 
  open(GRAPHOUTPUT,">$output_file") || die "Can't open $output_file: $!\n";
  select((select(GRAPHOUTPUT), $| = 1)[$[]);   # unbuffer file output

  
  #data to write the header file 
  $xlabel = "Time(s)";
  if ($readme_title_exists == 1) { 
      $title = $readme_title . "  " . $date; 
  } else { 
      $title = $date;
  }
  @params = keys %outputparameter;
  foreach $param (@params) { 
    if ($param =~ /(IS|IR|JT|AJ|AD|DL)/) { 
      $ylabel = "Secs";
    } elsif ($param =~ /(SQ|DR)/ ) { 
      $ylabel = "Sequence Numbers"; 
    } elsif ($param =~ /(SR|RR)/ ){
      $ylabel = "Mbps";
    } elsif ($param =~ /LN/) { 
      $ylabel = "Bytes";
    }
  }  
  
  #write the header 
  print GRAPHOUTPUT "TitleText: $title \n";
  print GRAPHOUTPUT "Markers: 1\n";
  print GRAPHOUTPUT "YLowLimit: 0\n";
  print GRAPHOUTPUT "XLowLimit: 0\n";
  print GRAPHOUTPUT "XUnitText: $xlabel\n";
  print GRAPHOUTPUT "YUnitText: $ylabel\n";
  print GRAPHOUTPUT "Device: Postscript\n";
  print GRAPHOUTPUT "Disposition: To File\n";
  $count = 0; 
  @params = keys %outputparameter;
  foreach $param (@params) { 
      $mycolor = $color{$param}; 
      print GRAPHOUTPUT "$count.Color: $mycolor\n";
      $count++;
  }
  print GRAPHOUTPUT "\n";
  print GRAPHOUTPUT "\n";

    
  @params = keys %outputparameter;
  foreach $param (@params) { 

      if ($param =~ /IS/) { 
	$legend = "Inter-departure Times"; 
      } elsif ($param =~ /IR/ ){
	$legend = "Inter-arrival Times"; 
      } elsif ($param =~ /SQ/) { 
	$legend = "Packets Sent"; 
      } elsif ($param =~ /AD/) { 
	$legend = "Average Delay"; 
      } elsif ($param =~ /AJ/) { 
	$legend = "Average Jitter"; 
      } elsif ($param =~ /JT/) { 
	$legend = "Jitter"; 
      } elsif ($param =~ /SR/ ){
	$legend = "Send Rate"; 
      } elsif ($param =~ /RR/) { 
	$legend = "Receiver Rate"; 
      } elsif ($param =~ /LN/) { 
	$legend = "Packet Lengths"; 
      } elsif ($param =~ /DL/) { 
	$legend = "Delay"; 
      } elsif ($param =~ /DR/) { 
	$legend = "Packet Drops"; 
      }
      
      
      print GRAPHOUTPUT "\n\n\"$legend\"\n"; 
      @timestamps = sort {$a <=> $b}  keys %{$stats{$param}};
      $max = $#timestamps;
      for ($index = 0; $index <= $max; $index++){
	$instance = $timestamps[$index];
	$value = $stats{$param}{$instance}; 
	if ( $param =~ /DR/ ) {
	  $value = $reflect_drops * $value; 
	}
	print GRAPHOUTPUT "$instance $value \n";
      }
  } # for each....

  close(GRAPHOUTPUT); 

}

#
# for gnuplot we generate two separate files <output filename>.dem and
# <output filename>.dat. The .dem file contains the command to display
# the data contained in the .dat file. 
# the format of the .dat file is simple 
#  <timestamp>  - - - - <value> - - - - - 
#
# Every parameter (eg. DR, SQ) is associated with a column. The - in
# other columns is used to indicate the fact that for the given
# timestamp the variable (whose column as non - entry) is the only one
# with valid data. Ideally when multiple datasets are selected, the
# set of timestamps is the union of the sets of timestamps of all the
# variables. Since the process of merging itself can be time
# consuming, we dont do it here. the algorithm implemented below is
# very simple. For each variable/parameter, obtain all the timestamps
# and add entries to the output file corresponding to each of the
# timestamps. Now if multiple variables/parameters are selected and
# they happen to have entries with the same timestamp, they will
# multiple entries with the same timestamp. gnuplot seems smart enough
# to display them "overlapped" in somesense - gnuplot is effectively
# doing the merging that we dont do. 
#
#
sub generate_gnuplot_output() {

  $output_file = $options{"-o"}; 
  open(PLOTFILE,">$output_file.dem") || die "Can't open $output_file: $!\n";
  select((select(PLOTFILE), $| = 1)[$[]);   # unbuffer file output
  open(DATFILE,">$output_file.dat") || die "Can't open $output_file: $!\n";
  select((select(DATFILE), $| = 1)[$[]);   # unbuffer file output
  
  $xlabel = "Time(s)";

  @params = keys %outputparameter;
  foreach $param (@params) { 
    if ($param =~ /(IS|IR|JT|AD|AJ|DL)/) { 
      $ylabel = "Secs";
    } elsif ($param =~ /(SQ|DR)/ ) { 
      $ylabel = "Sequence Numbers"; 
    } elsif ($param =~ /(SR|RR)/ ){
      $ylabel = "Mbps";
    } elsif ($param =~ /LN/) { 
      $ylabel = "Bytes";
    }
  }  
  
  if ($readme_title_exists == 1) { 
      $title = $readme_title . "  " . $date; 
  } else { 
      $title = $date;
      
  }

  print PLOTFILE "set title \'$title\' \n";
  print PLOTFILE "set xlabel \'$xlabel\'\n";
  print PLOTFILE"set ylabel \'$ylabel\'\n";
  print PLOTFILE "set nogrid\n";
  print PLOTFILE "set key left box\n";
#  print PLOTFILE "set data style lines \n";  
  print PLOTFILE "\n";
  print PLOTFILE "\n";
  
  @params = keys %outputparameter;
  $maxparams = $#params;
  if ($maxparams <  0 ) {
      print STDERR "Error. have to specify atleast one attribute to log \n"; 
      exit(1); 
  }


  #fix the positions of the variables in the matrix  
  $pos{"AD"} = 1; 
  $pos{"AJ"} = 2; 
  $pos{"DL"} = 3; 
  $pos{"DR"} = 4; 
  $pos{"IR"} = 5; 
  $pos{"IS"} = 6; 
  $pos{"JT"} = 7; 
  $pos{"LN"} = 8; 
  $pos{"RR"} = 9; 
  $pos{"SQ"} = 10; 
  $pos{"SR"} = 11; 

  foreach $param (@params) { 
      
      #print "Processing $param"; 
      #"$instance $valAD $valAJ $valDL $valDR $valIR $valIS $valJT $valLN  $valRR $valSQ $valSR\n"; 

      if ($param =~ /AD/) { 
          $format = "%f %f - - - - - - - - - -\n"; 
      } elsif ($param =~ /AJ/) { 
          $format = "%f - %f - - - - - - - - -\n"; 
      } elsif ($param =~ /DL/) { 
          $format = "%f - - %f - - - - - - - -\n"; 
      } elsif ($param =~ /DR/) { 
          $format = "%f - - - %f - - - - - - -\n"; 
      } elsif ($param =~ /IR/) { 
          $format = "%f - - - - %f - - - - - -\n"; 
      } elsif ($param =~ /IS/) { 
          $format = "%f - - - - - %f - - - - -\n"; 
      } elsif ($param =~ /JT/ ){
          $format = "%f - - - - - - %f - - - -\n"; 
      } elsif ($param =~ /LN/) { 
          $format = "%f - - - - - - - %f - - -\n"; 
      } elsif ($param =~ /RR/) { 
          $format = "%f - - - - - - - - %f - -\n"; 
      } elsif ($param =~ /SQ/) { 
          $format = "%f - - - - - - - - - %f -\n"; 
      } elsif ($param =~ /SR/) { 
          $format = "%f - - - - - - - - - - %6.7f\n"; 
      } 

      @times = sort {$a <=> $b}  keys %{$stats{$param}};
      $maxtimes = $#times;
      for ($index = 0; $index <= $maxtimes; $index++){      
	  $instance = $times[$index];      
	  $val = $stats{$param}{$instance}; 
	  
	  #put data in incremental order. the order matters...
	  printf(DATFILE $format, $instance, $val); 
      }
      
  }


  print PLOTFILE "plot "; 
  @params = sort {$a <=> $b} keys %outputparameter;
  $count = 2; 
  foreach $param (@params) { 
      
    $column = $pos{$param} + 1; 
    #print "Selecting $param in column $column\n"; 

    if ($param =~ /SQ/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Sequence Numbers\' with points"; 
      $count++;
    } elsif ($param =~ /IR/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Inter-arrival Times at Receiver\' with points"; 
      $count++;
    } elsif ($param =~ /IS/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Inter-arrival Times at Sender\' with points"; 
      $count++;
    } elsif ($param =~ /SR/ ){

      if ($count > 2) {
	print PLOTFILE ",\\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Send Rate\' with points"; 
      $count++;
    } elsif ($param =~ /RR/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Receiver Rate\' with points"; 
      $count++;
    } elsif ($param =~ /LN/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Packet Lengths\' with points"; 
      $count++;
    } elsif ($param =~ /DR/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Packet Drops\' with points"; 
      $count++;
    } elsif ($param =~ /DL/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Delay\' with points"; 
      $count++;
    } elsif ($param =~ /AD/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Average Delay\' with points"; 
      $count++;
    } elsif ($param =~ /AJ/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Average Jitter\' with points"; 
      $count++;
    } elsif ($param =~ /JT/) { 
      if ($count > 2) {
	print PLOTFILE ", \\\n\t"; 	  # first put a new line
      }
      print PLOTFILE "\'$output_file.dat\' using 1:$column t \'Jitter\' with points"; 
      $count++;
    } 

  }
  print PLOTFILE "\n\npause -1 \'Press Enter\'\n"; 

  close(DATFILE); 
  close(PLOTFILE); 

}

#
# xplot mixes both the plotting commands as well as the data that must
# be plotted. As a result, gengraph.pl has large control over what
# gets displayed and where. 
#
#
sub generate_xplot_output() {

  $output_file = $options{"-o"}; 
  open(GRAPHOUTPUT,">$output_file") || die "Can't open $output_file: $!\n";
  select((select(GRAPHOUTPUT), $| = 1)[$[]);   # unbuffer file output
  
  $xlabel = "Time(s)";
  @params = keys %outputparameter;
  foreach $param (@params) { 
    if ($param =~ /(AD|AJ|DL|JT)/ ) { 
	$ylabel = "Secs"; 
    } elsif ($param =~ /(SQ|DR)/ ) { 
      $ylabel = "Sequence Numbers"; 
    } elsif ($param =~ /(IR|IS)/) { 
      $ylabel = "Secs";
    } elsif ($param =~ /(SR|RR)/ ){
      $ylabel = "Mbps";
    } elsif ($param =~ /LN/) { 
      $ylabel = "Bytes";
    }
  }  

  if ($readme_title_exists == 1) { 
      $title = $readme_title . "  " . $date ; 
  } else { 
      $title = $date;
  }

  print GRAPHOUTPUT "double double\n";
  print GRAPHOUTPUT "title\n";
  print GRAPHOUTPUT "$title\n";    
  print GRAPHOUTPUT "xlabel\n"; 
  print GRAPHOUTPUT "$xlabel\n";
  print GRAPHOUTPUT "ylabel\n"; 
  print GRAPHOUTPUT "$ylabel\n";
  print GRAPHOUTPUT "white\n";
  print GRAPHOUTPUT "\n";

  $pos_max_x = 0; 
  $pos_max_y = 0; 
  $pos_min_x = 999999999999; 
  $pos_min_y = 999999999999; 


  @params = keys %outputparameter;
  $numparams = $#params; 
  foreach $param (@params) { 
    
    @keys = sort {$a <=> $b}  keys %{$stats{$param}};
    $max = $#keys;

    #skip any initial negative keys. shouldnt actually be
    #necessary. but retained for historical reasons.
    $index = 0; 
    $prevkey = $keys[$index]; 
    for ($index = 0; $index <= $max && $prevkey <= 0 ; $index++){
      $prevkey = $keys[$index]; 
    }
    $prevstat = $stats{$param}{$prevkey};
      
    for ( ; $index <= $max; $index++){
      $instance = $keys[$index];
      $value = $stats{$param}{$instance}; 
	

      if ($instance > $pos_max_x ) {$pos_max_x = $instance;}
      if ($instance < $pos_min_x ) {$pos_min_x = $instance;}
      if ($value > $pos_max_y ) {$pos_max_y = $value;}
      if ($value < $pos_min_y ) {$pos_min_y = $value;}
      
      #only drops must be treated differently 
      $mycolor = $color{$param}; 
      if ($param =~ /DR/ ) {
	
	if ($numparams == 0 ) { 
	  #this is the only variable asked to be plotted.
	  $new = $value;
	} else {
	  $new = $reflect_drops * $value;
	}
	print GRAPHOUTPUT "diamond $instance $new\n";
	print GRAPHOUTPUT "$mycolor\n";
      } else { 
	print GRAPHOUTPUT "plus $instance $value\n";
	print GRAPHOUTPUT "$mycolor\n";
      }
      
      $prevkey = $instance; 
      $prevstat = $value; 
    }

  } #for each       
  

  #figure out where to put the legend pos_max_x, pos_max_y contain the
  #top right hand side corner. these are set of the default values if
  #there is no data to be output. 

  if (($pos_min_y eq 999999999999) && ($pos_max_y eq 0)) { 
    $pos_max_y = 10.0; 
    $pos_min_y = 0.0; 
    $vertoff = 1.0; 
  } else { 
    $vertoff = ($pos_max_y - $pos_min_y)/20; 
  }
  
  @params = keys %outputparameter;
  foreach $param (@params) { 

    $pos_max_y -= $vertoff;
    $mycolor = $color{$param}; 

    if ($param =~ /AD/) { 
      $legend = "Average Delay"; 
    } elsif ($param =~ /AJ/) { 
      $legend = "Average Jitter"; 
    } elsif ($param =~ /JT/) { 
      $legend = "Jitter"; 
    } elsif ($param =~ /SQ/) { 
      $legend = "Sequence Numbers"; 
    } elsif ($param =~ /IS/) { 
      $legend = "Inter-arrival Times at Sender"; 
    } elsif ($param =~ /IR/) { 
      $legend = "Inter-arrival Times ar Receiver"; 
    } elsif ($param =~ /SR/ ){
      $legend = "Send Rate"; 
    } elsif ($param =~ /RR/) { 
      $legend = "Receiver Rate"; 
    } elsif ($param =~ /LN/) { 
      $legend = "Packet Lengths"; 
    } elsif ($param =~ /DR/) { 
      $legend = "Packet Drops"; 
    }

    print GRAPHOUTPUT "ltext $pos_max_x $pos_max_y $mycolor\n$legend\n";
  }
  
  close(GRAPHOUTPUT);

}


#select the output graph format based the arguments. 
sub generateoutput () {
  
  if ($outputformat =~ /xgraph/) {
    &generate_xgraph_output(); 
  } elsif ($outputformat =~ /xplot/) { 
    &generate_xplot_output();     
  } elsif ($outputformat =~ /gnuplot/) { 
    &generate_gnuplot_output();     
  }

}

