#!/usr/bin/perl
#
# should plot the comtech modem variables: ebno, freq, signal
# start                            lstolp 20110926
#
# took plot program from oceanus comtech modem and customized for
# atlantis. Plotting EbNo, AGC, BER  not sure about BER it seems to 
# remain at zero. To change time span, change the number, currently 
# only works in 24's.. 
#   /ships/plot_modem_dy.pl?span=48
#
##                                 lstolp 20120207




#$Debug=1;

use FileHandle;
use CGI qw(:standard :html3);
require "flush.pl";

#$span = url_param('span');

$span=48;
print STDERR "\$span = $span\n" if ($Debug);

$ship="atlantis";

# path and directory structure
my $HTTP_DIR = "/var/www";
my $httproot = "$HTTP_DIR/html/";
my $imagedir = "timages/";
my %head = ();

## tmp dir (make if it does not exist)
$tmp_dir = "/tmp/csv";
   system("mkdir -p $tmp_dir");



## time span (in seconds)
my $time_span = $span*3600;

print STDERR "\$time_span = $time_span\n" if ($Debug);

## get start time/end time

my $end_epoch_sec = time;
my @end_time_array = gmtime($end_epoch_sec);

my $start_epoch_sec = $end_epoch_sec - $time_span;
my @start_time_array = gmtime($start_epoch_sec);
my $out_file = "dac_$$.txt";

##
## get the ship id from the CRUISE_ID
##

my $cruise_id = `cat /home/data/CRUISE_ID`;
my $ship_id = substr($cruise_id,0,2);


#######################################################################

# Get today's date in the same format as athena's naming convention.

$datestr = `date '+%a %b %d %T %Z %Y %B %m %y %A %-d' -u`; chomp $datestr;

# this returns something like the following:
# Thu Dec 03 23:25:56 UTC 1998 December 12 98 Thursday 3

($dow,
 $mon,
 $day,
 $hour,
 $min,
 $sec,
 $zone,
 $year,
 $monthname,
 $monthnum,
 $yr,
 $weekday,
 $shortday) = split (/[ ]+|:/, $datestr);

$enddate = $yr.$monthnum.$day;
$Enddate = $year.$monthnum.$day;

print STDERR "\$enddate = $enddate\n" if ($Debug);


#this method lets date deal with the y2k problem
$ydnow = `date '+%j' -u`;chomp $ydnow;

$startdate = `date -d '$howmuch days ago' -u '+%y%m%d'`; chomp $startdate;
$Startdate = `date -d '$howmuch days ago' -u '+%Y%m%d'`; chomp $startdate;

chomp $Startdate;

# the following returns a string of the form 224241 to represent the time to
# start reading the file.

$starttime = `date -d '$howmuch days ago' -u '+%H%M%S'`;
chomp $starttime;
$endtime = $starttime;

 $start_datetime_str = "$Startdate"."$starttime";
 $end_datetime_str = "$Enddate"."$endtime" ;

 print STDERR "GET \$start_datetime_str = $start_datetime_str\n" if ($Debug);
 print STDERR "GET \$end_datetime_str = $end_datetime_str\n" if ($Debug);

    ##
    ## make list of data files to be read
    ##


    my $file_list;

    my $data_dir = "/home/shipdata/hsn/";


     my @files = `ls -1 $data_dir`;

    for my $file (@files){
        if ($file =~ /$ship_id(\d{6})_hsn-DAC\.csv/i)
        {   print "\$file = $file\n" if ($Debug);
            if ($1 >= $startdate && $1 <= $enddate)
            {
                push (@file_list, "$data_dir/$file");
            }
        }
  }





    ## get data from files
    ##

        ## filehandle hash for header output files
    ## for each csv file

    open (OUT,">> $tmp_dir/$out_file") or die "cannot create $out_file";
      print OUT "Date Time gyro_degrees AzRel_degrees AzTrue_degrees El_degrees Cr_mystery DacAgc_counts Aux_counts\n";

    for my $file (@file_list){
        if (! open(CSV_FILE, $file) ){
            die "Unable to open file '$csv_file'.  Error message was: $!\n";
        }

       ##
        ## process headers
        ##
        print STDERR "\$file = $file \n" if ($Debug);

        ## strip header line of leading/trailing whitespace
        my $header_line = <CSV_FILE>;
        my $header_line = strip($header_line);


        ## split headers
        my @headers = split /,/, $header_line;
        my $row;

       ## create a hash of the header info to figure out which column it is
        ##
           foreach $header (@headers)
              {
                   my $header = strip($header);
                   $header =~ tr/[A-Z]/[a-z]/;
                   $head{$header} = $row;
                   $row++;
              }


# this is mostly a debug statement to see what your hash looks like
while ( my ($key, $value) = each(%head) ) {
        print "$key => $value\n" if $Debug;
      }


        print STDERR "\$head{\"el\"} = $head{el} \n" if ($Debug);
        print STDERR "\$head{\"Gyro\"} = $head{gyro} \n" if ($Debug);
        print STDERR "\$head{\"aux\"} = $head{aux} \n" if ($Debug);

        ##
        ## process CSV data lines
        ##

        while($line = <CSV_FILE>){

            my @data = split(/,/, $line);

            ##
            ## check if data is in the time span
            ##

            ## convert date/time to a datetime string for use in comparisons

            my ($date, $time) = (strip($data[0]), strip($data[1]));

            ## create gnuplot date,time sting
             ($yr , $month, $day) = split(/\//, $date);
              my $year = substr($yr, 2, 2);


            my $date_plot = join ("/", $month, $day, $year);

            my $plot_time = join ("/", $date_plot, $time);

            my $date_str = join("",split(/\//, $date));
            my $time_str = join("",split(/:/, $time));

            my $datetime_str = join("", $date_str, $time_str);

            ## go to next line if not in time span
            if ( ($datetime_str < $start_datetime_str) ||
                 ($datetime_str > $end_datetime_str)
                 ){
                next;
            }

            ##
            ## write data to tmp files
            ##

            if ($head{"gyro"})
               {
               $gyro = $data[$head{"gyro"}];
               } else {
               $gyro = 0;
               }

         if ($head{"azrl"})
               {
               $azrel = $data[$head{"azrl"}];
               } else {
               $azrel = 0;
               }

         if ($head{"aztru"})
               {
               $aztrue = $data[$head{"aztru"}];
               } else {
               $aztrue = 0;
               }


         if ($head{"el"})
               {
               $el = $data[$head{"el"}];
               } else {
               $el = 0;
               }

         if ($head{"cr"})
               {
               $cr = $data[$head{"cr"}];
               } else {
               $cr = 0;
               }
 
         if ($head{"dcagc"})
               {
               $dacagc = $data[$head{"dcagc"}];
               } else {
               $dacagc = 0;
               }
 
         if ($head{"aux"})
               {
               $aux = $data[$head{"aux"}];
               } else {
               $aux = 0;
               }



        print OUT "$date $time $gyro $azrel $aztrue $el $cr $dacagc $aux\n";
   }


close(CSV_FILE);
}
close(OUT);

# Write the gnuplot command file.
my $datafile = "$tmp_dir/$out_file" ;

print STDERR "\$gyro  = $gyro \n \$aux =$aux\n" if ($Debug);


print "Content-type: text/html\n";
print "Pragma: no-cache\n\n";
 print <<EOM;
<HTML>
<head> <title> Atlantis HSN Plots </title> </head>

<frameset rows="*">
<frame src="http://www.atlantis.whoi.edu/cgi-bin/dygraphs/plot_csv.pl?fileinfo=1&f=$tmp_dir/$out_file&width=1000&fill=1&title=$shipname">
</frameset>
</HTML>
EOM







########## subroutines  ########################

## remove whitespace
sub strip{
    my @out = @_;
    for (@out) {
        s/^\s+//;
        s/\s+$//;
    }
    return wantarray ? @out : $out[0];
  }


