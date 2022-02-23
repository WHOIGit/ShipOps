//---------------------------------------------------------------------------
//
// This code was provided by Shawn R. Smith and Mark A. Bourassa
// 
// The original code is included in the SVN deirectory developmentDocumentation.
// 
// A good amount of 'cleanup' was performed during the original integration effort.  
// The original code was setup for batch processing data sets with large tabular formatted
// output routines with printfs.  Additionally, there were several arguments to the
// main calculation function that were more appropriately handled as local variables.
// There is a section of header comments below where the details of these changes
// are described.
// 
//---------------------------------------------------------------------------
// Revision history:
// 
// 2014-06-06   tt   - Integrated into dsLogCsv
// 
// 
//---------------------------------------------------------------------------
/*
   DETAILS ON MODIFICATIONS DURING INITIAL INTEGRATION

 - eliminate definition and calls to output procedures.
 - eliminate arguments nw, nwam, nwpm, nwf. No need to track statistics if not in batch mode.
 - convert all array arguments to single floats. 
 - eliminate loop over i.
 - eliminate wmis argument.  Function will not be called with missing args.
 - eliminate num and sel arguments to function (obsolete).
 - add {} braces for all if and else code statements
 - update commenting style.
 - add result argument (type windspeed_result_t) an eliminate tdir, adir and tspd args.
 - change all 'float's to 'double's.
 - update header comments below.
 - 2014-07-30 - modify code to convert cspd from knots to m/s

 */
//---------------------------------------------------------------------------
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

 crse	double      	Course TOWARD WHICH the vessel is moving over the 
                        ground. Referenced to true north and the fixed earth.
 cspd	double      	Speed of vessel over the ground. Referenced
			to the fixed earth.
 hd	double      	Heading toward which bow of vessel is pointing. 
			Referenced to true north.
 wdir	double      	Wind direction measured by anemometer,
 			referenced to the ship.
 wspd	double      	Wind speed measured by anemometer,referenced to
			the vessel's frame of reference.
 zlr	double		Zero line reference -- angle between bow and
 			zero line on anemometer.  Direction is clockwise
			from the bow.  (Use bow=0 degrees as default 
 			when reference not known.)			

 *** WDIR MUST BE METEOROLOGICAL (DIRECTION FROM)! CRSE AND CSPD MUST BE
     RELATIVE TO A FIXED EARTH! ***

 OUTPUT VALUES:

 speed	double      	True wind speed - referenced to the fixed earth.

 direction double      	True wind direction - referenced to true north
			and the fixed earth with a direction from which 
 			the wind is blowing (meteorological).

 apparentDirection	double
                       	Apparent wind direction (direction measured by
			wind vane, relative to true north). IS 
                        REFERENCED TO TRUE NORTH & IS DIRECTION FROM
			WHICH THE WIND IS BLOWING. Apparent wind 
                        edirection is the sum of the ship relative wind 
			direction (measured by wind vane relative to the
                        bow), the ship's heading, and the zero-line
			reference angle.  NOTE:  The apparent wind speed
			has a magnitude equal to the wind speed measured
			by the anemometer (wspd).

********************************************************************************
*/



#include<stdio.h>
#include<stdlib.h>
#include <math.h>
#include "truewind_SmithBourassa.h"


void truewind_SmithBourassa(windspeed_result_t *result, double crse, double cspd, double hd, double wdir, double wspd, double zlr)
{

  // Define variables.
  int calm_flag;
  double x, y, mcrse, mwdir, mtdir, dtor;
   
  // Initialize values.
  x = 0;
  y = 0;
  dtor = pi/180;  // degrees to radians conversion

  /*    Check course, ship speed, heading, wind direction, and wind speed
	for valid values (i.e. neither missing nor outside physically 
	acceptable ranges) . */
  if( ( (crse < 0) || (crse > 360) )
      || ( cspd < 0 )
      || ( (wdir < 0) || (wdir > 360) )
      || ( wspd < 0  )
      || ( (hd < 0) || (hd > 360) ) ) {         
    // When some or all of input data fails range check, true winds are set to missing. Step index for input value(s) being out of range
    result->direction = -9999.0;
    result->speed = -9999.0;
  } 

  // When course, ship speed, heading, wind direction, and wind speed are all in range, then compute true winds.
  else {
    // convert from navigational coordinates to angles commonly used in mathematics
    mcrse = 90.0 - crse;   

    // keep the value between 0 and 360 degrees
    if( mcrse <= 0.0 ) { mcrse = mcrse + 360.0; }

    // check zlr for valid value.  If not valid, set equal to zero.
    if( (zlr < 0.0) || (zlr > 360.0) ) { zlr = 0.0; }
	 
    // calculate apparent wind direction
    result->apparentDirection = hd + wdir + zlr;
	 
    // keep result->apparentDirection between 0 and 360 degrees
    while( result->apparentDirection >= 360.0 ) { result->apparentDirection = result->apparentDirection - 360.0; }

    // convert from meteorological coordinates to angles commonly used in mathematics
    mwdir = 270.0 - result->apparentDirection;

    // keep the value between 0 and 360 degrees
    if( mwdir <= 0.0 ) {
      mwdir = mwdir + 360.0;
    }
    else if( mwdir > 360.0 ) {
      mwdir = mwdir - 360.0;
    }

    // Convert ship speed from knots to m/s.
    // This conversion was not in the original C code - but is present in the perl script.
    // SSSG wants truewind reported in m/s so the conversion is needed.
    //
    // 1 knot = 0.514 m/s
    cspd = cspd * 0.514;

    // determined the East-West vector component and the North-South vector component of the true wind
    x = (wspd * cos(mwdir * dtor)) + (cspd * cos(mcrse * dtor));
    y = (wspd * sin(mwdir * dtor)) + (cspd * sin(mcrse * dtor));

    // use the two vector components to calculate the true wind speed
    result->speed = sqrt((x * x) + (y * y));
    calm_flag = 1;

    // determine the angle for the true wind
    if(fabs(x) > 0.00001) {
      mtdir = (atan2(y,x)) / dtor;
    }
    else { 
      if(fabs(y) > 0.00001) {
	mtdir = 180.0 - (90.0 * y) / fabs(y);
      }
      else {
	// the true wind speed is essentially zero: winds are calm and direction is not well defined
	mtdir = 270.0;
	calm_flag = 0;
      }
    }

    // convert from the common mathematical angle coordinate to the meteorological wind direction
    result->direction = 270.0 - mtdir;

    // make sure that the true wind angle is between 0 and 360 degrees
    while(result->direction < 0.0) { result->direction = (result->direction + 360.0) * calm_flag; }
    while(result->direction > 360.0) {result->direction = (result->direction - 360.0) * calm_flag; }

  }  // end of large else block to handle good values
}

/*********************************************************************/
