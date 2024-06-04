#!/usr/bin/perl


#  this is from Alex's make_sparklines.pl
#
#  This program will hopefully plot all  the underway data
#  depending on how it is called.
#  currently 
# june 24 2009 First run taking data from .cvs file for starboard
#              and port GPS sensor...         Walter Cacho
#                                                     lstolp
#
# October 2, 2009 Turned the header into a hash, variable to number
#                 where number is the column of the data 
#                                                     lstolp
#
#
# January 2010    Ignore case of header, turning every thing to lower case.
#                 If the variable is empty do not use it, before it was making
#                 it zero, but this is for the gmt maps and if there is no data, 
#                 then do not need any.
#
# 20190429        updated for Atlantis to make gmt.xy files in shiptrack, will
#                 turn these into kml files

# 
#$Debug = 1;

use FileHandle;

my $out_file = "gmt.xy";
my $output_dir = "/home/data/shiptrack/";
my $last_out="$output_dir/last.xy";
my $label="$output_dir/gmt.label";
my $minmax="minmax";


print "\$out_file = $out_file\n" if ($Debug);
print "\$output_dir = $output_dir\n" if ($Debug);

##
## get the ship id from the CRUISE_ID
##

my $cruise_id = `cat /home/data/CRUISE_ID`;
my $ship_id = substr($cruise_id,0,2);
my %head;


    ## make list of data files to be read
    ##

    my @csv_file_list;
    my $data_dir = "/home/data/underway/proc";
    my @files = `ls -1 $data_dir`;

    for my $file (@files){
        if ($file =~ /$ship_id(\d{6})_.*\.csv/i){
                push (@csv_file_list, "$data_dir/$file");
            }
        }

    ##
    ## extract data from files
    ##

    ## filehandle hash for header output files
    ## for each csv file

    open (OUT,"> $output_dir/$out_file") or die "cannot create $output_dir/$out_file";

    for my $csv_file (@csv_file_list){
        if (! open(CSV_FILE, $csv_file) ){
            die "Unable to open file '$csv_file'.  Error message was: $!\n";
        }

        my $row = 0;
        ##
        ## process headers
        ##
        print STDERR "\$csv_file = $csv_file \n" if ($Debug);
        ## throw away title line
        <CSV_FILE>;

        ## strip header line of leading/trailing whitespace
        my $header_line = <CSV_FILE>;
        my $header_line = strip($header_line);

        ## split headers
        my @headers = split /,/, $header_line;

        ## create a hash of the header info to figure out which column it is
        ## 
        print "\@headers = @headers\n" if ($Debug);

           foreach $header (@headers)
              {
                   my $header = strip($header);
                   $header =~ tr/[A-Z]/[a-z]/;
                   $head{$header} = $row;
                   $row++;
              }

        print "\$head{\"dec_lat\"} = $head{dec_lat} \n" if ($Debug);

        ##
        ## process CSV data lines
        ##

        while($line = <CSV_FILE>){


            my @data = split(/,/, $line);

            my ($date, $time) = (strip($data[0]), strip($data[1]));

            ## create date,time string
             ($yr , $month, $day) = split(/\//, $date);
              my $year = substr($yr, 2, 2);


            my $date_plot = join ("/", $month, $day, $year);

            my $plot_time = join ("/", $date_plot, $time);


            ##
            ## write data to xy files if gps data is missing, do not write to the file
            ##    
         if ( ($data[$head{"dec_lon"}] != " ") || ($data[$head{"dec_lat"}] != " ") )
                  {
                           printf OUT "%10.4f, %10.4f, %10s, %10s\n", $data[$head{"dec_lon"}], $data[$head{"dec_lat"}], $time, $date;
                           $lat  =  $data[$head{"dec_lat"}];
                           $lon  =  $data[$head{"dec_lon"}];
                           $now  =  $data[$head{"date"}];

                           # get minmax arrays
                             push(@lat,$lat);
                             push(@lon,$lon);
                   }
        }
        close(CSV_FILE);
   }
        close(OUT);



# find the min and max value and write to a file
 @order_lat = sort { $a <=> $b} @lat;
 $lah = $order_lat[$#order_lat];
 $lal = $order_lat[0];

 @order_lon = sort { $a <=> $b} @lon;
 $lol = $order_lon[0];
 $loh = $order_lon[$#order_lon];


# Write minmax file
open (MINMAX, ">$output_dir/$minmax");
print MINMAX "$lal, $lah, $lol, $loh\n";
close MINMAX;

# Write last out
open (LASTOUT,">$last_out") or die "cannot open $last_out";
print LASTOUT "$lon $lat $now $now_t \n";
close LASTOUT;


# Write LABEL file

open (LABEL,">$label") or die "cannot open $label";
print LABEL "$lon $lat 16 0 6 1 $cruise_id\n";

if ($lat < 0)
{
 $latc = abs($lat);
 $NS = "S";
}
else
{
 $latc = $lat;
 $NS = "N";
}
if ($lon < 0)
{
 $lonc = abs($lon);
 $EW = "W";
}
else
{
 $lonc = $lon;
 $EW = "E";
}

$lastmsg = "Last position: $latc $NS, $lonc $EW at $now_t gmt on $now\n";
print LABEL "$lon $lat 14 0 4 1 $lastmsg\n";

## remove whitespace
sub strip{
    my @out = @_;
    for (@out) {
        s/^\s+//;
        s/\s+$//;
    }
    return wantarray ? @out : $out[0];
  }






