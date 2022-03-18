//---------------------------------------------------------------------------
//
// This code was provided by Shawn R. Smith and Mark A. Bourassa
// 
// The original code is included in the SVN deirectory developmentDocumentation.
// 
//---------------------------------------------------------------------------
// Revision history:
// 
// 2014-06-06   tt   - Integrated into dsLogCsv
// 
//---------------------------------------------------------------------------

#define pi acos(-1.)

//-----------------------------------------------------------------------------------
typedef struct {
  double speed;
  double direction;
  double apparentDirection;
} windspeed_result_t;

//-----------------------------------------------------------------------------------
void truewind_SmithBourassa(windspeed_result_t *result, double crse, double cspd, double hd, double wdir, double wspd, double zlr);
