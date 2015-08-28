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
my $t_dir = '/tmp/derp/';
my $t_file = 'running';
if(defined $ARGV[0]){
	my $new_temp = $ARGV[0];
	if($new_temp =~/^\d+$/){
		
		if ( !-e $t_dir.$t_file ) {
			if ( !-e $t_dir ) {
				`mkdir $t_dir`;;	
			}
			`touch $t_dir$t_file`;
		}

		`echo -n $new_temp > $t_dir$t_file`;
		print "New set temp is: ".`cat $t_dir$t_file`."*c\n";
		
	}else{
		help();
	}


}else{
	help();
}

sub help{
	print "Usage: .$0 temp_in_c \n";
}
