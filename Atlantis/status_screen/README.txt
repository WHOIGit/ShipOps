2015-Feb-04

send_dslog_ssv.pl - which sent the ssv info to the UDP port 55504 to be eventually
    sent to em122was disabled

write_svt.pl - which wrote the ssv data to the serial port that went to the em122
    has been disabled

These were replaced by the dsLogCSV - which now has an OUTPUT that creates the UDP broadcast
    and send it to FTP
	&
    redirect_svt_from_UDP.pl - which reads this broadcast and send it to the serial port
             /dev/ttyS1 for the em122


