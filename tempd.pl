#!/usr/bin/perl

    # Set up the serial port
    use Device::SerialPort;
    use File::Slurp;
    use warnings;
	use strict;
	use sigtrap qw/handler signal_handler normal-signals stack-trace error-signals/;

    my @dmesg = split /\n/, `dmesg`;
    my $tty;


	foreach my $t ( glob '/dev/ttyUSB*'  ) {
		if(-e $t){
			$tty = $t;
		}
	}

	if(!defined($tty)){
		die "Could not find any serial devices..";
	}

	print "Trying to open tty: $tty\n";
    my $port = Device::SerialPort->new($tty)
		or die "Cant open $tty ";
	print "Connection to $tty a success!\n";
    
    
    use Sys::Syslog;                          # Misses setlogsock.
    use Sys::Syslog qw(:DEFAULT setlogsock);  # Also gets setlogsock.
    my $logopt = ""; # Options for sysslog
    my $facility = "local0";
    my $priority = "info";
    
    
     
    print "Serial config start \n"; 
    # 9600, 81N on the USB ftdi driver
    $port->baudrate(9600); # you may change this value
    $port->databits(8); # but not this and the two following
    $port->parity("none");
    $port->stopbits(1);
    print "Serial config done. \n"; 
     
    # now catch gremlins at start
    my $tEnd = time()+2; # 2 seconds in future
    while (time()< $tEnd) { # end latest after 2 seconds
      my $c = $port->lookfor(); # char or nothing
      next if $c eq ""; # restart if noting
      print $c; # uncomment if you want to see the gremlin
      last;
    }
    
    my $i=0;
    
    while (1) {
	my $hold_temp; # New temperature to hold
	my $pwr_sw_time; # Lock time for changing the power relay. As not to cause harm to physical equipment, and as a timing method.
	my $fn_variables = "/tmp/derp/running";
	
	# Check to see if new input variables has arrived
	if(-e $fn_variables and -w $fn_variables){
	    my $tmp = `cat $fn_variables`;
	    chomp $tmp;
	    # Sanity check the input variables
	    if($tmp =~/^(.*?)\s+(.*)$/){
			$hold_temp = $1;
			$pwr_sw_time = $2;
			my $output = $hold_temp . " " . $pwr_sw_time."\n";
			print "[tempd.pl] Writing $output \n";
			$port->write($output);
			print "[tempd.pl] Writing done.\n";
	    }
	    unlink $fn_variables;	
	}
    # Poll to see if any data is coming in

	print "[tempd.pl] Polling serial..\n";
    my $char = $port->lookfor();
	print "[tempd.pl] Polling done.\n"; 
        
    
    # If we get data, then print it
    if ($char) {
        my $input = "$char \n";
		print " Input: $input \n";
        if($input !~ /Temp/g){
            next;
            }
        my $date = `date`;
        chomp $date;
        my $output = $input;
#        print $output;
        write_file("/tmp/temp.log", $output);
        
        my $ident = "Project_c";    
        
        setlogsock({ type => "udp", host => "localhost", port => "1234"});
        openlog($ident, $logopt, $facility);    # don't forget this
        syslog($priority, $output);        
        closelog();
    
        $i++;
    }
	sleep(1);
    
    # Uncomment the following lines, for slower reading,
    # but lower CPU usage, and to avoid
    # buffer overflow due to sleep function.
 
    #$port->lookclear;
    #sleep (1);
    }

   sub signal_handler{
	print "Trying to close serial port\n";
	$port->close || warn "close failed";
	die "[templ.pl] Closing down. Signal: $!";
   
   } 
