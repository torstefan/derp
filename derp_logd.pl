#!/usr/bin/env perl 
#===============================================================================
#
#         FILE: derp_logd.pl
#
#        USAGE: ./derp_logd.pl  
#
#  DESCRIPTION: Derp Logg daemon. Starts up all the logging
#
#      OPTIONS: ---
# REQUIREMENTS: ---
#         BUGS: ---
#        NOTES: ---
#       AUTHOR: YOUR NAME (), 
# ORGANIZATION: 
#      VERSION: 1.0
#      CREATED: 12/08/15 21:46:57
#     REVISION: ---
#===============================================================================

use strict;
use warnings;
use utf8;
use IPC::System::Simple qw(system capture);

print "Checking status Splunk \n";
my $splunk_status = `sudo /etc/init.d/splunk status`;
print $splunk_status;
if($splunk_status =~/not/xm){
	print `sudo /etc/init.d/splunk start`;
}


print "Connecting to Project C\n";
while(1){
	eval{
		system($^X, "tempd.pl", "");
	};
	print "\n";
	sleep(1);
}
