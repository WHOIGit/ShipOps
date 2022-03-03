#!/usr/bin/perl

#  to run:
#        mk_cruise_meta.pl  {input file - cruise letter} 
#  output file will be called cruise.metadata 

#  input file:  the cruise letter - probably something like at##-##_cruise_letter
#                       that is exported from email and copied to /home/data
#  purpose is to provide a standard formatted cruise.metadata file to be included with the
#  cruise data.  Some items like start time and end time may need to be added manually but
#  hopefully this script will accomplish most of the work.
#  written by A. Heater - disclaimer: the cruise letter format is assumed to be rather standard
#                         if it is not, this program will fill in some incorrect information

# VESSEL NAME IS ON LINE 48

##################
#THE STANDARD FORMAT IS (without the "#" of course):
#
#Vessel: R/V Atlantis
#Institution: Woods Hole Oceanographic Institution
#Cruise:
#Start Port: CITY, STATE
#Start Date: YYYY/MM/DD HH:MM ZONE
#End Port: CITY, STATE
#End Date: YYYY/MM/DD HH:MM ZONE
#Chief Scientist: NAME (INST)
#Project:
#Area:
#Captain:
#SSSG:
#Participants: NAME (INST), NAME (INST)
#
#Notes:
############################
#set some of the values needed 
#that do not change between cruises

$Debug=0;

###############DELETE THE EMPTY cruise.metadata FILE - if one exists
#otherwise this will just be appended at the bottom
system ("rm -rf cruise.metadata");

############################

#line1
$vessel1 = 'Vessel: R/V Atlantis';

#line2
$inst1 = 'Institution: Woods Hole Oceanographic Institution';

################################
#initialize some variables for counting the names
#and making a list

     #need to count up how many names there are
     $cnt_name = 0;

     #start an initially empty string of names
     $name_list = ' ';

     #initilize marker for finding line starting with "Voyage" 
     $voy1 = 'N';
     $date1 = 'N';

##########################
###########################
#open the input file
#my $fin = (<>);

my $fin = $ARGV[0] || "";;

print STDERR "The value of the input is $fin\n" if ($Debug);

open IN, "<$fin" or die "$0: couldn't open $fin\n\t";

################################
##############READ THROUGH THE CRUISE LETTER - input data file
while (<IN>) 
{
  
#print STDERR "NEW LINE - loop flags are \$voy1 is $voy1 and \$date1 is $date1\n" if ($Debug);

    @words = split(' ',$_);

    #split also on the either of teh two different sorts of dashes that have appeared
    #in start/end date/port
    if ($_ =~ m/-/)
    {
      @voyage = split(/-/,$_);
    }
    else
    {
       if ($_ =~ m/–/)
       {
         @voyage = split(/–/,$_);
       }
    }

#how many words are in this line?
    $n = scalar @words;

#Look for the header pieces that give the information needed for the cruise.metadata file
#example line: Captain Allan Lunt
 
########LOOK FOR CAPTAIN
#name might have more than two parts - name start in position words[1]
     if ($words[0] eq 'Captain')  
     {
         $capt1 = ' ';
         $cntc = 1;
         while ($cntc < $n)
         {
            $capt1 = $capt1.' '.$words[$cntc];
            $cntc = $cntc + 1;
         }
         $capt1 = "Captain: ".$capt1;
print STDERR "Captain is $capt1\n" if ($Debug);
     }

#########DONE LOOK FOR CAPTAIN

######GET START/END DATES AND PORTS 
#these are the first two lines following the line starting with "Voyage"
#that are not blank lines

     #if either flag is 'F' we are looking for a date or port
     if ((($voy1 eq 'F') || ($date1 eq 'F')) && ($n >= 1))
     {
         #which is it?  date or port? 
         #if $voyage[0] includes a number - assume it is a date
         if ($voyage[0] =~ m/[0-9]/)
         {
print STDERR "I found a number\n" if ($Debug);
         $start_day = $words[0];
         $len1 = length($start_day);
         if ($len1 eq 1)
         {
         $start_day = "0".$start_day;
         }
         $amon1 = $words[1];
         $start_year = $words[2];
         $end_day = $words[4];
         if (length($end_day) eq 1)
         {
         $end_day = "0".$end_day;
         }

         $amon2 = $words[5];
         $end_year = $words[6];

   #set the month - start
   if ($amon1 eq 'January') {$mon1 = "01";}
   if ($amon1 eq 'February') {$mon1 = "02";} 
   if ($amon1 eq 'March') {$mon1 = "03";} 
   if ($amon1 eq 'April') {$mon1 = "04";} 
   if ($amon1 eq 'May') {$mon1 = "05";} 
   if ($amon1 eq 'June') {$mon1 = "06";} 
   if ($amon1 eq 'July') {$mon1 = "07";} 
   if ($amon1 eq 'August') {$mon1 = "08";} 
   if ($amon1 eq 'September') {$mon1 = "09";} 
   if ($amon1 eq 'October') {$mon1 = "10";} 
   if ($amon1 eq 'November') {$mon1 = "11";} 
   if ($amon1 eq 'December') {$mon1 = "12";} 

   #set the month - end 
   if ($amon2 eq 'January') {$mon2 = "01";}
   if ($amon2 eq 'February') {$mon2 = "02";}   
   if ($amon2 eq 'March') {$mon2 = "03";}   
   if ($amon2 eq 'April') {$mon2 = "04";}   
   if ($amon2 eq 'May') {$mon2 = "05";}   
   if ($amon2 eq 'June') {$mon2 = "06";}   
   if ($amon2 eq 'July') {$mon2 = "07";}   
   if ($amon2 eq 'August') {$mon2 = "08";}   
   if ($amon2 eq 'September') {$mon2 = "09";}   
   if ($amon2 eq 'October') {$mon2 = "10";}   
   if ($amon2 eq 'November') {$mon2 = "11";}   
   if ($amon2 eq 'December') {$mon2 = "12";}   

   $begin1 = $start_year.'/'.$mon1.'/'.$start_day;
   $end1 = $end_year.'/'.$mon2.'/'.$end_day;

   $sd1 = "Start Date: $begin1 08:00 GMT";
   $ed1 = "End Date: $end1 08:00 GMT";

print STDERR "cruise start is $begin1\n" if ($Debug);
print STDERR "cruise end is $end1\n" if ($Debug);

     $date1 = 'N';
         #end loop dealing with date
         }
         else
         {
           $sp1 = "Start Port: $voyage[0]";
           $ep1 = "End Port: $voyage[1]";          
           chop($ep1);
           $voy1 = 'N';
 
print STDERR "It must be the port names\n" if ($Debug);
print STDERR "The first part of voyage is $voyage[0]\n" if ($Debug);
print STDERR "The second part of voyage is $voyage[1]\n" if ($Debug);

         }
      #end loop looking for port/date start/end
      }
######END GET THE START AND END PORTS

#####GET THE CRUISE NUMBER
     if ($words[0] eq 'Voyage')
     {
         $cru1 = $words[1];
         $cru1 =~ s/#//;
         $num1 = $words[2];
         
         $cruise1 = "Cruise: $cru1$num1";
#found the word Voyage as first word, assume next two lines are 
#the cruise dates and the start and end ports.  Set flags to enter
#loops to grab those pieces of data
         $voy1 = 'F';
         $date1 = 'F';
     }
########END GET THE CRUISE NUMBER

########GET THE LIST OF NAMES OF PARTICIPANTS, CHIEF AND SSSGs

     #find the first comma position, then assemble the name portion and the institution portion, then add to list
     if (($words[0] eq 'Dr.') || ($words[0] eq 'Ms.') || ($words[0] eq 'Mr.') || ($words[0] eq 'Mrs.'))
     #LOOP A
     {
         #this matched the name criteria so increment the name count
         $cnt_name = $cnt_name + 1;
         $found_comma = 'N';

         #how many pieces to the name? which word has the comma?
         #only really want to count the first comma that occurs
         $cnt1 = 1;

         while (($cnt1 < $n) && ($found_comma eq 'N'))
         {
            #find the end of the name
            if ($words[$cnt1] =~ /,/)
            {
               #found a comma at words[$cnt1], set flag to exit while and mark comma location
               $found_comma = 'F';
               $comma_mark1 = $cnt1;
#print STDERR "found the comma in $words[$cnt1]\n" if ($Debug);
#print STDERR "that is words position $comma_mark1\n" if ($Debug);

            }
            #didn't find comma yet and need to check next word
            $cnt1 = $cnt1 + 1;
#print STDERR "no comma or exited loop where I found comma\n\n if ($Debug)";
         }

         # assemble sections name and institute now
         $cnt2 = 0;

         while ($cnt2 <= n)
         {
               $first = ' ';
               #loop to assemble first name
               while ($cnt2 <= $comma_mark1)
               {
                 $first = $first.' '.$words[$cnt2]; 
                 $cnt2 = $cnt2 + 1;
#print STDERR "assembling the name, it is $first so far\n\n" if ($Debug);
               }
               $first =~ s/,//;
#print STDERR "the final name from this line is, $first\n\n" if ($Debug);
               # loop to assemble institution portion 
               $inst_part = '(';
               while ($cnt2 <= $n)
               {
                 $inst_part = $inst_part.' '.$words[$cnt2];
                 $cnt2 = $cnt2 + 1;
#print STDERR "assembling institution, it is $inst_part so far\n\n" if ($Debug);
               }
               #need a unusual symbol to look for to resplit list - might be too many commas - use "%"
               $inst_part = $inst_part.')%,';

#print STDERR "institution is  $inst_part\n\n" if ($Debug);
         }

     #add the name to the list of names
     $name_list = $name_list.$first.' '.$inst_part;

     }
     #END LOOP A
}
#end of the while loop
########END GET THE NAMES OF PARTICIPANTS, ETC

########FIND THE SSSGs and CH SCI
#could probably have done this inside the previous section, but oh well!
print "TOTAL NAMES  $cnt_name \n\n" if ($Debug);
print "All the names are  $name_list\n\n" if ($Debug);

#Assume the last two names are the SSSG's
@get_sssg = split('%,',$name_list);

#re-assemble into science and sssg lists, starting with the name after the ch sci
  $cnt3 = 1;

#assume Chief Sci is the first name 
  $chsci = "Chief Scientist: $get_sssg[0]";
  $sci_list = "Participants: ";

  while ($cnt3 < $cnt_name - 3)
  {
     $sci_list = $sci_list.$get_sssg[$cnt3].',';
     $cnt3 = $cnt3 + 1;
  }
  $sci_list = $sci_list.$get_sssg[$cnt3];

# remove the WHOI part from the techs - if the tech is from somewhere else it will remain
  $sssg1 = $get_sssg[$cnt3 + 1];
  @fix_sssg1 = split('\(',$sssg1);
  $sssg1 = $fix_sssg1[0];

  $sssg2 = $get_sssg[$cnt3 + 2];
  @fix_sssg2 = split('\(',$sssg2);
  $sssg2 = $fix_sssg2[0];

  $sssg_list = "SSSG: $sssg1 and $sssg2";

print "Science list is  $sci_list\n" if ($Debug);
print "SSSG list is  $sssg_list\n" if ($Debug);

#END FIND THE SSSGs and CH SCI
######################################

my $fout = "cruise.metadata";
open OUT, ">> $fout" or die "cannot create $fout";

#print out the header

print OUT ("$vessel1\n");
print OUT ("$inst1\n");
print OUT ("$cruise1\n");
print OUT ("$sp1\n");
print OUT ("$sd1\n");
print OUT ("$ep1\n");
print OUT ("$ed1\n");
print OUT ("$chsci\n");
print OUT ("Project:  \n");
print OUT ("Area:  \n");
print OUT ("$capt1\n");
print OUT ("$sssg_list\n");
print OUT ("$sci_list\n");
print OUT ("\n");
print OUT ("Notes:\n");
close(OUT);
close(IN);
