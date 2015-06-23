#!/usr/bin/perl

    # Set up the serial port
    use Device::SerialPort;
    use File::Slurp;
    use warnings;

    my $dmesg = `dmesg | tail`;
    my $tty;
    if($dmesg =~/attached\sto\s(ttyUSB\d)/mx){

	$tty = $1;
    }
	print "Tty: $tty\n";
    my $port = Device::SerialPort->new("/dev/$tty");
    
    
    use Sys::Syslog;                          # Misses setlogsock.
    use Sys::Syslog qw(:DEFAULT setlogsock);  # Also gets setlogsock.
    my $logopt = ""; # Options for sysslog
    my $facility = "local0";
    my $priority = "info";
    
    
     
     
    # 9600, 81N on the USB ftdi driver
    $port->baudrate(9600); # you may change this value
    $port->databits(8); # but not this and the two following
    $port->parity("none");
    $port->stopbits(1);
     
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
	my $fn_variables = "/tmp/project_c";
	
	# Check to see if new input variables has arrived
	if(-e $fn_variables and -w $fn_variables){
	    my $tmp = `cat $fn_variables`;
	    chomp $tmp;
	    # Sanity check the input variables
	    if($tmp =~/^(.*?)\s+(.*)$/){
			$hold_temp = $1;
			$pwr_sw_time = $2;
			my $output = $hold_temp . " " . $pwr_sw_time."\n";
			$port->write($output);
			print "[tempd.pl] Writing $output";
	    }
	    unlink $fn_variables;	
	}
    # Poll to see if any data is coming in
    my $char = $port->lookfor();
        
    
    # If we get data, then print it
    if ($char) {
        my $input = "$char \n";
	print "Input: $input \n";
        if($input !~ /Time=/g){
            next;
            }
        my $date = `date`;
        chomp $date;
        my $output = $input;
        print $output;
        write_file("/tmp/temp.log", $output);
        
        my $ident = "Project_c";    
        
        setlogsock({ type => "udp", host => "localhost", port => "1234"});
        openlog($ident, $logopt, $facility);    # don't forget this
        syslog($priority, $output);        
        closelog();
    
        $i++;
    }
    
    # Uncomment the following lines, for slower reading,
    # but lower CPU usage, and to avoid
    # buffer overflow due to sleep function.
 
    #$port->lookclear;
    #sleep (1);
    }

    
