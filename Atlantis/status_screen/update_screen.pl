#!/usr/bin/perl 
#
# 20110304  lstolp   took out -w  and debug
#                    commented out 
# update_screen.pl

use DBI;
use CGI qw(:standard);
use Time::localtime;
use POSIX qw(strftime);
use CGI::Pretty qw( :html3 );
use Getopt::Long;

#$debug = 1;
$db_name = "/usr/lib/cgi-bin/db_driven_data/status_screen/sqlite_database/datascreen.db";
$ship = "Atlantis";
@months = ('Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec');

GetOptions('debug' => \$debug);
GetOptions('database=s' => \$db_name);

$title_font = "<font color=white><font size=5><b>";
$green_font = "<font color=lime><font size=5><b>";
$white_font = "<font color=lightblue><font size=5><b>";
$yellow_font = "<font color=yellow><font size=5><b>";
$orange_font = "<font color=orange><font size=5><b>";
$red_font = "<font color=red><font size=5><b>";
$black_font = "<font color=black></b>";

$dbh = DBI->connect( "dbi:SQLite:". $db_name, $username, $password, { PrintError => 0} ) || die "Can not connect.";

print STDERR "Connected: ", $dbh, "\n" if $debug;

$table_result = $dbh->do( "CREATE TABLE latest_data (
              header  VARCHAR(10) PRIMARY KEY ,
              udp_time VARCHAR (10),
              data VARCHAR (180),
              timeEnter VARCHAR(120))" );

if ($debug) {
  if (!defined($table_result)) {
        print STDERR "Table already created\n";
  } else {
        print STDERR "Table created\n";
  }
}

$trigger_result = $dbh->do("CREATE TRIGGER insert_data_timeEnter AFTER INSERT ON latest_data
        BEGIN
            UPDATE latest_data SET timeEnter = strftime ('%s', 'now')
                        WHERE rowid = new.rowid;
        END");

if ($debug) {
  if (!defined($trigger_result)) {
        print STDERR "Trigger already created\n";
  } else {
        print STDERR "Trigger created\n";
  }
}

open (config_file, "screen.config") || die "Can't find config file.";

$time = localtime;
$epoch_time = time;

print header(-refresh=>'15'), start_html(-http_equiv=>"Pragma",
		-content=>"no-cache", -title=> "$ship Underway Data",
		-bgcolor => 'black');

print start_html("Underway Data");

#
# Seems that hour and minute are enough on the time: otherwise clock gets too far off on a
#	display that only updates every 15 seconds
#
#printf "<p>$white_font %02d:%02d:%02d GMT $black_font\n", $time->hour, $time->min, $time->sec;
#
print  "$white_font $ship Underway Data $black_font \n";
printf "&nbsp&nbsp&nbsp $white_font %02d:%02d GMT &nbsp&nbsp&nbsp", $time->hour, $time->min;
printf "$white_font %s %1d, %d $black_font", $months[$time->mon], $time->mday, 1900+$time->year;

#print '<table border="0" cellpadding="1" cellspacing="1 style=font-family:sans-serif">';
#
#$title = "$white_font $ship Underway Data $black_font";
#$timeofday = sprintf "&nbsp&nbsp&nbsp $white_font %02d:%02d GMT &nbsp&nbsp&nbsp", $time->hour, $time->min;
#$date = sprintf "$white_font %s %1d, %d $black_font", $months[$time->mon], $time->mday, 1900+$time->year;

#print Tr( td({-align=>"left"}, $title), td("&nbsp&nbsp"),
#	td({-align=>"center"}, $timeofday), td("&nbsp&nbsp"),
#        td({-align=>"right"}, $date)), td("&nbsp&nbsp"), "\n";
#print '<\table>\n';

print '<p>';
print '<table border="0" cellpadding="1" cellspacing="1 style=font-family:sans-serif">';

while (($config_line = <config_file>)) {
	chop ($config_line);
	print STDERR "Line from config file: ", join( '|', split(':', $config_line)), "\n" if $debug;	

	($label_left, $key_left, $field_no_left, $format_string_left, $special_case_left,
		$label_right, $key_right, $field_no_right, $format_string_right, $special_case_right)
		= split(':', $config_line);

	$label = $label_left;
	$key = $key_left;
	$field_no = $field_no_left;
	$format_string = $format_string_left;
	$special_case = $special_case_left;

	($fstart_left, $label_left, $data_value_left) = make_entry();

	$label = $label_right;
	$key = $key_right;
	$field_no = $field_no_right;
	$format_string = $format_string_right;
	$special_case = $special_case_right;

	($fstart_right, $label_right, $data_value_right) = make_entry();

	$fend = $black_font;
	print Tr( td({-align=>"right"},$fstart_left.$label_left.$fend), td("&nbsp&nbsp"),
			td({-align=>"left"}, $white_font.$data_value_left." ".$fend), td("&nbsp&nbsp"),
		td({-align=>"right"},$fstart_right.$label_right.$fend), td("&nbsp&nbsp"),
                        td({-align=>"left"}, $white_font.$data_value_right." ".$fend)), "\n";

}
print "</TABLE>";

print end_html;


sub make_entry {

	$label = $label.":";

	print STDERR "Make_entry called\r label $label key $key field no $field_no \n",
		"    format $format_string case $special_case \n" if $debug;
	
	$dbi_instr = $dbh->prepare("SELECT * FROM latest_data WHERE header = '$key' ");
        $dbi_instr->execute( );

        @db_data = $dbi_instr->fetchrow_array;
        print STDERR "\ndb_data: $key \n$db_data[0]\n$db_data[1]\n$db_data[2]\n$db_data[3]\n" if $debug;
	@data_fields = split (/,/, $db_data[2]);

	print STDERR "Special case field is: $special_case \n" if $debug;

	if ($special_case eq "") {
		print STDERR "Found normal case\n" if $debug;
		print STDERR join('|',@data_fields), "\n" if $debug;
		print STDERR "Field: $field_no, Data: $data_fields[$field_no]\n" if $debug;
		$data_value = sprintf ( $format_string, $data_fields[$field_no]);
	}
	elsif ($special_case eq "blank") {
		print STDERR "Found blank case\n" if $debug;
		$label = "";
		$data_value = "";
	}		
	elsif ($special_case eq "longitude") {
		print STDERR "Found longitude case\n" if $debug;
		$nav_degree = substr($data_fields[$field_no], 0, 3);
		$nav_minute = substr($data_fields[$field_no], 3, 8);
		$nav_letter = $data_fields[$field_no+1];
		$data_value = sprintf ($format_string, $nav_degree, $nav_minute, $nav_letter);
	}
	elsif ($special_case eq "latitude") {
		print STDERR "Found latitude case\n" if $debug;
		$nav_degree = substr($data_fields[$field_no], 0, 2);
		$nav_minute = substr($data_fields[$field_no], 2, 8);
		$nav_letter = $data_fields[$field_no+1];
		$data_value = sprintf ($format_string, $nav_degree, $nav_minute, $nav_letter);
	}
	elsif ($special_case eq "airtemp") {
		print STDERR "Found airtemp case\n" if $debug;
		$ctemp = $data_fields[$field_no];
		$ftemp = 32.0 +( 9.0 * $ctemp) / 5.0;
		$data_value = sprintf ($format_string, $ctemp, $ftemp);
	}
	elsif ($special_case eq "hrsmin") {
		print STDERR "Found hrsmin case\n" if $debug;
		$hrsmin_time = $data_fields[$field_no];
		$hrsmin_hr = int $hrsmin_time;
		$hrsmin_min = 60. * ($hrsmin_time - $hrsmin_hr);
		$format_string = join (':', split (';', $format_string));
		if ($hrsmin_time < 0.05) {
			$format_string = $format_string."*";
		}
		$data_value = sprintf($format_string, $hrsmin_hr, $hrsmin_min);
		if ($hrsmin_time < 0.) {
			$data_value = "n/a";
		}
	}
	elsif ($special_case eq "netconn") {
		print STDERR "Found netconn case\n" if $debug;
		$data_value = $data_fields[$field_no];
		$data_value = ($data_value eq "Connected") 
					? $green_font.$data_value
					: $red_font.$data_value;
	}

        elsif ($special_case eq "knots") {
              print STDERR "Found knots case\n" if $debug;
              $spdms = $data_fields[$field_no];
              $spdkn = ($spdms * 1.94);
              $data_value = sprintf($format_string, $spdms, $spdkn);
        }


	print STDERR "Data value, format string, data:\n$data_value $format_string $data_fields[$field_no]\n" if $debug;
# change to yellow at one minute, orange at 2 minutes and red at 5 minutes
	$age = $epoch_time - $db_data[3];
	$fstart = $age > 300
		? $red_font
		  : $age > 120
       	            ? $orange_font
       	              : $age > 60 
       	                ? $yellow_font
       	                  : $green_font;
	print STDERR "\n $age $fstart \n" if $debug;


	if (($age > 900) && ($data_value ne "")) {
		print STDERR "Old, non-blank data: Age: $age Data value: $data_value\n" if $debug;
		$data_value = "No data";
	}
	print STDERR "At make_entry return: Fstart: $fstart Label: $label Data: $data_value\n" if $debug;
	return ($fstart, $label, $data_value);
}
