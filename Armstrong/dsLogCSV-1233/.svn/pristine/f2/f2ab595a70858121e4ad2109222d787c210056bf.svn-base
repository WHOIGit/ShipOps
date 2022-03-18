/*  Program:  test.c
    Developed by: Shawn R. Smith and Mark A. Bourassa
    Programmed by: Mylene Remigio
    Purpose:  testing of program truewind.c
    Updated:  February 12, 1998
   
    Direct questions to:  wocemet@coaps.fsu.edu 
     
    DESCRIPTION
    ***********
    
    The following source code enables the user to test the truewind.c
    program using sixteen examples:
    --Examples #1-10 contain no missing values, and all values
      are valid.  "tdir" and "tspd" are calculated.  
    --Example #11 contains one missing value.  "tdir" and "tspd" are
      NOT calculated and are given the values of the missing value
      flag.
    --Example #12 contains all missing values.  "tdir" and "tspd" are
      NOT calculated and are given the values of the missing value
      flag.  
    --Examples #13-17 each contain one invalid value.  "tdir" and "tspd"
      are NOT calculated and are given the values of the missing value
      flag.
      
    Descriptions of the variable names can be found in the source code
    of "truewind.c".
    
    The output should be the similar to the text file "results".
    
    Because this data set is relatively small, the full test is run
    (sel = 1) to show all possible types of output.
 
    
    COMPILING AND RUNNING
    *********************
    
    First, compile "truewind.c" by typing at the prompt:
           cc -c truewind.c

    Next, to compile "test.c", type:
           cc -o test test.c -lm
           
    Then to run "test" simply type:
           test

*/


#include "truewind.c"

void main()
{

  int i, num, sel, nw, nwpm, nwam, nwf;
  float tdir[17], tspd[17], zlr;

  float crse[17] = {0.0, 0.0, 0.0, 0.0, 180.0, 90.0, 90.0, 225.0, 270.0, 0.0, 2.0, -1111.0, -2.0, 3.0, 0.0, 0.0, 5.0};
  float cspd[17] = {0.0, 0.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 3.0, 0.0, -9999.0, -9999.0, 8.0, -6.0, 0.0, 0.0, 1.0};
  float wdir[17] = {90.0, 90.0, 0.0, 0.0, 180.0, 90.0, 135.0, 270.0, 90.0, 0.0, 0.0, 1111.0, 8.0, 0.0, -9.0, 7.0, 2.0};
  float hd[17] = {0.0, 90.0, 0.0, 0.0, 180.0, 90.0, 45.0, 225.0, 270.0,  0.0, 0.0, 5555.0, 8.0, 8.0, 8.0, 8.0, -13.0};
  float wspd[17] = {5.0, 5.0, 5.0, 0.0, 5.0, 5.0, 5.0, 5.0, 4.0, 0.0, 0.0, 9999.0, 3.0, 3.0, 3.0, -4.0, 2.0};
  float adir[17] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  float wmis[5] = {-1111.0, -9999.0, 1111.0, 9999.0, 5555.0};
  
  zlr = 0.0;
  num = 17;
  sel = 1;

  truewind(num, sel, crse, cspd, wdir, zlr, hd, adir, wspd, wmis, tdir, tspd, nw, nwpm, nwam, nwf);

}
  
