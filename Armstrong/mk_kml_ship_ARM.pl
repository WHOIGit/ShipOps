#!/usr/bin/perl

# mk_kml_ship.pl
# Creates a KML file of the cruise tracks using the gmt.xy file created for gmt.
# KML file can be brought into google earth, trying to figure out how to make this
# work offline or as a background process. 
# 
# lstolp 20190429
#
#

$Debug = 1;




use CGI ':standard';

my $in_file = "gmt.xy";
my $dir = "/home/data/shiptrack";
my $file;

my $cruiseid = `cat /home/data/CRUISE_ID`;
chomp $cruiseid;

my $outfile = "$cruiseid.kml";
print "\$outfile = $outfile\n" if ($Debug);

opendir DATADIR, "$dir" or die "no data directory\n";
        $file = "$dir/$in_file";
        print "\$file = $file\n" if ($Debug); 
        open(OUT,">$dir/$outfile") or die "cannot create $dir/$outfile";

        print "\$dir/\$outfile = $dir/$outfile\n" if ($Debug);
        print OUT ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        print OUT ("<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n");
        print OUT ("<Document>\n");
        print OUT ("<Placemark>\n");
        print OUT ("<name> $crusieid </name>\n");
        print OUT ("<description>\n");
        print OUT ("<![CDATA[\n");
        print OUT ("<h5> $cruiseid </h5>\n");
        print OUT ("]]>\n");
        print OUT ("</description>\n");
        print OUT ("<Style>\n");
        print OUT ("      <LineStyle>\n");
        print OUT ("      <color>501400D2</color>\n");
        print OUT ("      </LineStyle>\n");
        print OUT (" </Style>\n");
        print OUT ("<LineString>\n");
        print OUT ("<coordinates>\n");



       get_data($file, $dir, $outfile);


      print OUT ("</coordinates>\n");
      print OUT ("</LineString>\n");
      print OUT ("</Placemark>\n");
      print OUT ("</Document>\n");
      print OUT ( "</kml>\n"); 


  close (OUT);


sub get_data
{
  print "\$file = $file\n" if ($Debug);

  open (IN, "$file") or die "nope no $file";
    
    while (<IN>)
       {
        ($lon, $lat, $time, $date) = split (/,/);
        print "$lon,$lat\n" if ($Debug);
        print "\$file = $file\n" if ($Debug);
        printf OUT ("%f,%f\n", $lon, $lat);
 
      }
  close (IN);
}


