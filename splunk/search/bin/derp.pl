#!/usr/bin/env perl 
#===============================================================================
#
#         FILE: derp.pl
#
#        USAGE: ./derp.pl  
#
#  DESCRIPTION: Writes variables from the derp syntax to /tmp/running as input to the temperature controller
#
#      OPTIONS: ---
# REQUIREMENTS: ---
#         BUGS: ---
#        NOTES: ---
#       AUTHOR: YOUR NAME (), 
# ORGANIZATION: 
#      VERSION: 1.0
#      CREATED: 21/06/15 23:33:09
#     REVISION: ---
#===============================================================================

use strict;
use warnings;
use utf8;

my $hold_temp = $ARGV[0];
my $pwr_pause_time = $ARGV[1];

if($hold_temp =~ m/\d+/xm and $pwr_pause_time =~ m/\d+/xm){
	if(! -d "/tmp/derp/"){
		`mkdir /tmp/derp/`;
		`chown -R root:derp  /tmp/derp/`;
		`chmod -R g+w /tmp/derp/`;

	}
	`echo "$hold_temp $pwr_pause_time" > /tmp/derp/running`;
	print "derp_hold_temp,derp_pause_time";
	print $hold_temp.",".$pwr_pause_time;
}

