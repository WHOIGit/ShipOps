/*

 Subprogram TRUEWIND.C      
 Converted from IDL true wind computation code true_wind.pro.

 Created: 12/17/96
 Comments updated: 10/29/97
 Developed by: Shawn R. Smith and Mark A. Bourassa
 Programmed by: Mylene Remigio
 Last updated:  02/13/98
 Direct questions to:  wocemet@coaps.fsu.edu 
 
 This routine will compute meteorological true winds (direction from
 which wind is blowing, relative to true north; and speed relative to
 the fixed earth).

 INPUT VALUES:

 num	int		Number of observations in input (crse, cspd, wdir,
 			wspd, hd) and output (adir, tdir, tspd) data
			arrays.  
 			ALL ARRAYS MUST BE OF EQUAL LENGTH.
 sel	int		Sets option for diagnostic output.  There are four
 			settings:

			Option 4:  Calculates true winds from input arrays 
                                   with no diagnostic output or warnings. 
                                   NOT RECOMMENDED.
			Option 3:  [DEFAULT] Diagnostic output lists the
                                   array index and corresponding variables 
                                   that either violate the range checks or are
                                   equal to the missing value. An additional
                                   table lists the number of observation times
                                   with no missing values, some (but not all)
                                   missing values, and all missing values; as
                                   well as similar totals for the observation
                                   times that fail the range checks.
                                   Range checks identify negative input
                                   values and verify directions to be between
                                   0 and 360 degrees.
			Option 2:  In addition to the default diagnostics
                                   (option 3), a table of all input and output
                                   values for observation times with missing 
                                   data is provided.
			Option 1:  Full diagnostics -- In addition to the 
                                   diagnostics provided by option 2 and 3, a
                                   complete data chart is output. The table 
                                   contains input and output values for all
                                   observation times passed to truewind.

 crse	float array	Course TOWARD WHICH the vessel is moving over the 
                        ground. Referenced to true north and the fixed earth.
 cspd	float array	Speed of vessel over the ground. Referenced
			to the fixed earth.
 hd	float array	Heading toward which bow of vessel is pointing. 
			Referenced to true north.
 zlr	float		Zero line reference -- angle between bow and
 			zero line on anemometer.  Direction is clockwise
			from the bow.  (Use bow=0 degrees as default 
 			when reference not known.)			
 wdir	float array	Wind direction measured by anemometer,
 			referenced to the ship.
 wspd	float array	Wind speed measured by anemometer,referenced to
			the vessel's frame of reference.
 wmis	float array	Five element array containing missing values for
			crse, cspd, wdir, wspd, and hd. In the output, the missing
                        value for tdir is identical to the missing value
                        specified in wmis for wdir. Similarly, tspd uses
			the missing value assigned to wmis for wspd.

 *** WDIR MUST BE METEOROLOGICAL (DIRECTION FROM)! CRSE AND CSPD MUST BE
     RELATIVE TO A FIXED EARTH! ***

 OUTPUT VALUES:

 tdir	float array	True wind direction - referenced to true north
			and the fixed earth with a direction from which 
 			the wind is blowing (meteorological).
 tspd	float array	True wind speed - referenced to the fixed earth.
 adir	float array	Apparent wind direction (direction measured by
			wind vane, relative to true north). IS 
                        REFERENCED TO TRUE NORTH & IS DIRECTION FROM
			WHICH THE WIND IS BLOWING. Apparent wind 
                        edirection is the sum of the ship relative wind 
			direction (measured by wind vane relative to the
                        bow), the ship's heading, and the zero-line
			reference angle.  NOTE:  The apparent wind speed
			has a magnitude equal to the wind speed measured
			by the anemometer (wspd).

 DIAGNOSTIC OUTPUT:

 nw	int		Number of observation times for which tdir and
                        tspd were calculated (without missing values)
 nwpm	int		Number of observation times with some values
                        (crse, cspd, wdir, wspd, hd) missing.  tdir, 
			tspd set to missing value. 
 nwam	int		Number of observation times with all values
                        (crse, cspd, wdir, wspd, hd) missing. tdir, 
			tspd set to missing value.
 nwf	int		Number of observation times where the program
                        fails -- at least one of the values (crse, cspd,
                        wdir, wspd, hd) is invalid 

********************************************************************************
*/



#include<stdio.h>
#include<stdlib.h>
#include <math.h>
#include "truewind.h"


void truewind(int num, int sel, float crse[], float cspd[], float 
    wdir[], float zlr, float hd[], float adir[], float wspd[], float 
    wmis[5], float tdir[], float tspd[], int nw, int nwam, int nwpm, 
    int nwf)
{

/* Define variables. */
   
   int calm_flag;
   int i;
   float x, y, mcrse, mwdir, mtdir, dtor;
   
/* Initialize values.*/
   
   x = 0;
   y = 0;
   nw = 0;
   nwam = 0;
   nwpm = 0;
   nwf = 0;
   dtor = pi/180;  /* degrees to radians conversion  */

   printf("\n");
   
/* Loop over 'num' values. */

   for(i=0; i<num; i++)
     {

/*    Check course, ship speed, heading, wind direction, and wind speed
      for valid values (i.e. neither missing nor outside physically 
      acceptable ranges) . */

      if( ( ((crse[i] < 0) || (crse[i] > 360)) && (crse[i] != wmis[0]) ) ||
          ( (cspd[i] < 0) && (cspd[i] != wmis[1]) ) ||
          ( ((wdir[i] < 0) || (wdir[i] > 360)) && (wdir[i] != wmis[2]) ) ||
          ( (wspd[i] < 0) && (wspd[i] != wmis[3]) ) ||
	  ( ((hd[i] < 0) || (hd[i] > 360)) && (hd[i] != wmis[4]) ) )
        {         
/*       When some or all of input data fails range check, true winds are set
         to missing. Step index for input value(s) being out of range   */
         nwf = nwf + 1;
         tdir[i] = wmis[2];
         tspd[i] = wmis[3];   
             
         if( (crse[i] != wmis[0]) && (cspd[i] != wmis[1]) && 
             (wdir[i] != wmis[2]) && (wspd[i] != wmis[3]) &&
	     (hd[i] != wmis[4]) )
/*         Step index for all input values being non-missing  */  
           {
            nw = nw + 1;
           }
         else 
           { 
            if( (crse[i] != wmis[0]) || (cspd[i] != wmis[1]) || 
                (wdir[i] != wmis[2]) || (wspd[i] != wmis[3]) ||
		(hd[i] != wmis[4]) )
/*            Step index for part of input values being missing  */
              {
               nwpm = nwpm + 1;
              } 
            else
/*            Step index for all input values being missing  */
              {
               nwam = nwam + 1;
              }                
           }
        } 

/*    When course, ship speed, heading, wind direction, and wind speed 
      are all in range and non-missing, then compute true winds. */ 
         
      else if( (crse[i] != wmis[0]) && (cspd[i] != wmis[1]) && 
          (wdir[i] != wmis[2]) && (wspd[i] != wmis[3]) && 
	  (hd[i] != wmis[4]) )
        {
         nw = nw + 1;

/*       convert from navigational coordinates to angles commonly used 
         in mathematics  */
         mcrse = 90.0 - crse[i];   

/*       keep the value between 0 and 360 degrees  */
         if( mcrse <= 0.0 ) mcrse = mcrse + 360.0;

/*	 check zlr for valid value.  If not valid, set equal to zero.*/
	 if( (zlr < 0.0) || (zlr > 360.0) ) zlr = 0.0;
	 
/*	 calculate apparent wind direction  */
	 adir[i] = hd[i] + wdir[i] + zlr;
	 
/*	 keep adir between 0 and 360 degrees  */
	 while( adir[i] >= 360.0 ) adir[i] = adir[i] - 360.0;	 

/*       convert from meteorological coordinates to angles commonly used   
         in mathematics  */
         mwdir = 270.0 - adir[i];

/*       keep the value between 0 and 360 degrees  */
         if( mwdir <= 0.0 ) mwdir = mwdir + 360.0;
         else if( mwdir > 360.0 ) mwdir = mwdir - 360.0;

/*       determined the East-West vector component and the North-South
         vector component of the true wind */ 
         x = (wspd[i] * cos(mwdir * dtor)) + (cspd[i] 
             * cos(mcrse * dtor));
         y = (wspd[i] * sin(mwdir * dtor)) + (cspd[i] 
             * sin(mcrse * dtor));

/*       use the two vector components to calculate the true wind speed  */
         tspd[i] = sqrt((x * x) + (y * y));
         calm_flag = 1;

/*       determine the angle for the true wind  */
         if(fabs(x) > 0.00001) mtdir = (atan2(y,x)) / dtor;
         else 
           { 
            if(fabs(y) > 0.00001) mtdir = 180.0 - (90.0 * y) / fabs(y);
            else
/*            the true wind speed is essentially zero: winds are calm
              and direction is not well defined   */ 
              {
               mtdir = 270.0;
               calm_flag = 0;
              }
           }

/*       convert from the common mathematical angle coordinate to the 
         meteorological wind direction  */ 
         tdir[i] = 270.0 - mtdir;

/*       make sure that the true wind angle is between 0 and 360 degrees */ 
         while(tdir[i] < 0.0) tdir[i] = (tdir[i] + 360.0) * calm_flag;
         while(tdir[i] > 360.0) tdir[i] = (tdir[i] - 360.0) * calm_flag;

         x = 0.0; 
         y = 0.0;
        }

/*    When course, ship speed, apparent direction, and wind speed 
      are all in range but part of these input values are missing,
      then set true wind direction and speed to missing.  */  
         
      else
        {
         if( (crse[i] != wmis[0]) || (cspd[i] != wmis[1]) || 
             (wdir[i] != wmis[2]) || (wspd[i] != wmis[3]) ||
	     (hd[i] != wmis[4]) )
           {
            nwpm = nwpm + 1;
            tdir[i] = wmis[2];
            tspd[i] = wmis[3];
           }
            
/*       When course, ship speed, apparent direction, and wind speed 
         are all in range but all of these input values are missing,
         then set true wind direction and speed to missing.  */    

         else
           {
            nwam = nwam + 1;
            tdir[i] = wmis[2];
            tspd[i] = wmis[3];
           }
        }    
     }        

   
/* Output option selection for truewind.c  */
   
   switch(sel)
     {
      case 1:  full(num, crse, cspd, wdir, zlr, hd, adir, wspd, tdir, tspd);
               missing_values(num, crse, cspd, wdir, hd, wspd, tdir, 
               tspd, wmis);
	       truerr(num, crse, cspd, hd, wdir, wspd, wmis, nw, nwpm, 
                nwam, nwf);  
	       break;
      case 2:  missing_values(num, crse, cspd, wdir, hd, wspd, tdir, 
               tspd, wmis);
	       truerr(num, crse, cspd, hd, wdir, wspd, wmis, nw, nwpm, 
                nwam, nwf);  
	       break;
      case 3:  truerr(num, crse, cspd, hd, wdir, wspd, wmis, nw, nwpm, 
                nwam, nwf);     
	       break;
      case 4:  break;
      default: printf("Selection not valid.  Using selection #3 by default.\n");
      	       truerr(num, crse, cspd, hd, wdir, wspd, wmis, nw, nwpm, 
                nwam, nwf);
     }
}              

/*********************************************************************/
                  /*     OUTPUT PROCEDURES    */
/*********************************************************************/

/* Procedure:  FULL.C
     Purpose:  Produces a complete data table with all values. 
               Accessed only when selection #1 is chosen.
*/              

void full(int num, float crse[], float cspd[], float wdir[], float
    zlr, float hd[], float adir[], float wspd[], float tdir[], 
    float tspd[])
{

   int i;

   printf("------------------------------------------------------------------------------------\n");
   printf("\n                                   FULL TABLE\n");
   printf("                                  ************\n");
   printf("  index  course  sspeed  windir  zeroln  shiphd |  appspd |  appdir  trudir  truspd\n"); 

   for(i=0; i<num; i++)
     {
      printf("%7d %7.1f %7.1f %7.1f %7.1f %7.1f | %7.1f | %7.1f %7.1f %7.1f\n", (i+1),crse[i],cspd[i],wdir[i],zlr,hd[i],wspd[i],adir[i],tdir[i],tspd[i]);
     }

   printf("\n                   NOTE:  Wind speed measured by anemometer is identical\n");
   printf("                          to apparent wind speed (appspd).\n");
   printf("\n------------------------------------------------------------------------------------\n");
}

/*********************************************************************/

/* Procedure:  MISSING_VALUES.C
     Purpose:  Produces a data table of the data with missing values. 
               Accessed when selection #1 or #2 is chosen.
*/               

void missing_values(int num, float crse[], float cspd[], float wdir[], 
    float hd[], float wspd[], float tdir[], float tspd[],float wmis[])
{

   int i;
            
   printf("\n                               MISSING DATA TABLE\n");
   printf("                              ********************\n");
   printf("          index  course  sspeed  windir  shiphd  appspd  trudir  truspd\n");


   for(i=0; i<num; i++)
     {
      if( (crse[i] != wmis[0]) && (cspd[i] != wmis[1]) &&  
          (wdir[i] != wmis[2]) && (wspd[i] != wmis[3]) &&
	  (hd[i] != wmis[4]) )
        continue;
      else                   
        {
         printf("        %7d %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f\n", (i+1), crse[i], cspd[i], wdir[i], hd[i], wspd[i], tdir[i], tspd[i]);
        }
     }   


   printf("\n------------------------------------------------------------------------------------\n");
}

/*********************************************************************/

/* Procedure:  TRUERR.C
     Purpose:  List of where range tests fail and where values are
               invalid.  Also prints out number of records which are
               complete, partially incomplete, entirely incomplete, and
               where range tests fail.  Accessed when selection #1, #2,
               #3, or the default is chosen.
*/               

void truerr(int num, float crse[], float cspd[], float hd[], 
    float wdir[], float wspd[], float wmis[], int nw, int nwpm, 
    int nwam, int nwf)
{

   int i;
   
   printf("\n                               TRUEWINDS ERRORS\n");
   printf("                              ******************\n");


   for(i=0; i<num; i++)                
     {    
      if( ((crse[i] < 0) || (crse[i] > 360)) && (crse[i] != wmis[0]) )                                             
        printf("        Truewinds range test failed.  Course value #%d invalid.\n",(i+1));

      if( (cspd[i] < 0) && (cspd[i] != wmis[1]) )
        printf("        Truewinds range test failed.  Vessel speed value #%d invalid.\n",(i+1));

      if( ((wdir[i] < 0) || (wdir[i] > 360)) && (wdir[i] != wmis[2]) )     
        printf("        Truewinds range test failed.  Wind direction value #%d invalid.\n",(i+1));

      if( (wspd[i] < 0) && (wspd[i] != wmis[3]) ) 
        printf("        Truewinds range test failed.  Wind speed value #%d invalid.\n",(i+1));

      if( ((hd[i] < 0) || (hd[i] > 360)) && (hd[i] != wmis[4]) )                                             
        printf("        Truewinds range test failed.  Ship heading value #%d invalid.\n",(i+1));
     }

   printf("\n");

   for(i=0; i<num; i++)    
     { 
      if(crse[i] == wmis[0])      
        printf("        Truewinds data test:  Course value #%d missing.\n", (i+1));

      if(cspd[i] == wmis[1])
        printf("        Truewinds data test:  Vessel speed value #%d missing.\n", (i+1));

      if(wdir[i] == wmis[2])
        printf("        Truewinds data test:  Wind direction value #%d missing.\n", (i+1));

      if(wspd[i] == wmis[3])
        printf("        Truewinds data test:  Wind speed value #%d missing.\n", (i+1));

      if(hd[i] == wmis[4])
        printf("        Truewinds data test:  Ship heading value #%d missing.\n", (i+1));
     }    


   printf("\n------------------------------------------------------------------------------------\n");
   printf("\n                                 DATA REVIEW\n");
   printf("                                *************\n");
   printf("                            no data missing = %4d\n", nw);
   printf("                       part of data missing = %4d\n", nwpm);
   printf("                           all data missing = %4d\n", nwam);
   printf("                         failed range tests = %4d\n", nwf);       
}
