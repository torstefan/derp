#!/usr/bin/env perl 
#===============================================================================
#
#         FILE: tempdctl.pl
#
#        USAGE: ./tempdctl.pl  
#
#  DESCRIPTION: Controls the tempd , mainly setting new variables to the temp control logic.
#
#      OPTIONS: ---
# REQUIREMENTS: ---
#         BUGS: ---
#        NOTES: ---
#       AUTHOR: YOUR NAME (), 
# ORGANIZATION: 
#      VERSION: 1.0
#      CREATED: 20/07/15 17:32:34
#     REVISION: ---
#===============================================================================

use strict;
use warnings;
use utf8;
use Getopt::Long;
use File::Slurp;

my $holdTemp; # Tells the arduno which temperature to reach.
my $pwr_switch_pause_addition; # The pause time between switching the remote power on off.
my $acceptedChange;  # Change in temp need to be under this variable in order to switch the remote power on off


GetOptions ("ht|holdTemp=i" => \$holdTemp,    # numeric
"psp=i" => \$pwr_switch_pause_addition,
"ac|acceptedChange=f" => \$acceptedChange,
"h|help"=> \&help_msg
) or die "Hmm.. $!";

my $output = "";

if (! -d "/tmp/derp/"){
 `mkdir /tmp/derp`;
}

if (defined($holdTemp) | defined($pwr_switch_pause_addition) | defined($acceptedChange) ){
	print "holdTemp: $holdTemp, output: $output\n";
	defined $holdTemp					? $output .= $holdTemp." "					: $output .= "0 ";
	print "holdTemp: $holdTemp, output: $output\n";
	defined $pwr_switch_pause_addition	? $output .= $pwr_switch_pause_addition." " : $output .= "0 ";
	defined $acceptedChange				? $output .= $acceptedChange." " 			: $output .= "0.0";
	$output .= "\n";
	write_file("/tmp/derp/running", $output) or die "$!";
	print "Wrote $output to /tmp/derp/running\n";
}else{
	help_msg();
}






sub help_msg {
print 
'-ht|--holdTemp 		- Tells the arduno which temperature to reach.
--psp 					- The pause time between switching the remote power on off.
-ac|--acceptedChange 	- Change in temp need to be under this variable in order to switch the remote power on off
';

}


