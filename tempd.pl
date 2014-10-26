#!/usr/bin/perl

    # Set up the serial port
    use Device::SerialPort;
    use File::Slurp;
    my $port = Device::SerialPort->new("/dev/ttyUSB1");
    
    
    use Sys::Syslog;                          # Misses setlogsock.
    use Sys::Syslog qw(:DEFAULT setlogsock);  # Also gets setlogsock.
    my $logopt = ""; # Options for sysslog
    my $facility = "local0";
    my $priority = "info";
    
    
     
     
    # 19200, 81N on the USB ftdi driver
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
    # Poll to see if any data is coming in
    my $char = $port->lookfor();
        
    
    # If we get data, then print it
    # Send a number to the arduino
    if ($char) {
        my $input = "$char \n";
	print "Input: $input \n";
        $input =~/(\d{1,3}\.\d{2})/xm;
        my $temp = $1;
        if($temp !~ /\A\d/g){
            next;
            }
        my $date = `date`;
        chomp $date;
        my $output = "\"temp\","."\"".$date."\",\"".$temp."\"\n";
        print $output;
        #append_file("temp.log", $output);
        
        my $ident = "Project_c_temp_0";    
        
        setlogsock({ type => "udp", host => "localhost", port => "1234"});
        openlog($ident, $logopt, $facility);    # don't forget this
        syslog($priority, "temp=${temp}");        
        closelog();
    
        $i++;
    }
    
    # Uncomment the following lines, for slower reading,
    # but lower CPU usage, and to avoid
    # buffer overflow due to sleep function.
 
    #$port->lookclear;
    #sleep (1);
    }

    
