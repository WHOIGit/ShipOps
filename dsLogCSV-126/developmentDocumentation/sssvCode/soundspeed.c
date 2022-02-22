// Temp in degrees celcius
// Salinity in practical salinity units (ppt)
// Pressure in bars
//Functioning
double
ChenMillero (double sal, double temp, double press)
{
  // range of validity 0 - 40 Deg C
  // 0 - 40 ppt
  // 0 - 1000 bars
  double D;
  double B;
  double Cw;
  double SV;
  double A;

  double c00 = 1402.388;
  double c01 = 5.03830;
  double c02 = -5.81090e-2;
  double c03 = 3.3432e-4;
  double c04 = -1.47797e-6;
  double c05 = 3.1419e-9;
  double c10 = 0.153563;
  double c11 = 6.8999e-4;
  double c12 = -8.1829e-6;
  double c13 = 1.3632e-7;
  double c14 = -6.1260e-10;
  double c20 = 3.1260e-5;
  double c21 = -1.7111e-6;
  double c22 = 2.5986e-8;
  double c23 = -2.5353e-10;
  double c24 = 1.0415e-12;
  double c30 = -9.7729e-9;
  double c31 = 3.8513e-10;
  double c32 = -2.3654e-12;

  double a00 = 1.389;
  double a01 = -1.262e-2;
  double a02 = 7.166e-5;
  double a03 = 2.008e-6;
  double a04 = -3.21e-8;
  double a10 = 9.4742e-5;
  double a11 = -1.2583e-5;
  double a12 = -6.4928e-8;
  double a13 = 1.0515e-8;
  double a14 = -2.0142e-10;
  double a20 = -3.9064e-7;
  double a21 = 9.1061e-9;
  double a22 = -1.6009e-10;
  double a23 = 7.994e-12;
  double a30 = 1.100e-10;
  double a31 = 6.651e-12;
  double a32 = -3.391e-13;

  double b00 = -1.922e-2;
  double b01 = -4.42e-5;
  double b10 = 7.3637e-5;
  double b11 = 1.7950e-7;

  double d00 = 1.727e-3;
  double d10 = -7.9836e-6;

  D = d00 + (d10 * press);

  B = b00 + b01 * temp + (b10 + b11 * temp) * press;

  A =
    (a00 + a01 * temp + a02 * pow (temp, 2) + a03 * pow (temp, 3) +
     a04 * pow (temp, 4)) + (a10 + a11 * temp + a12 * pow (temp,
							   2) +
			     a13 * pow (temp, 3) + a14 * pow (temp,
							      4)) * press +
    (a20 + a21 * temp + a22 * pow (temp, 2) +
     a23 * pow (temp, 3)) * pow (press,
				 2) + (a30 + a31 * temp + a32 * pow (temp,
								     2)) *
    pow (press, 3);


  Cw =
    (c00 + c01 * temp + c02 * pow (temp, 2) + c03 * pow (temp, 3) +
     c04 * pow (temp, 4) + c05 * pow (temp,
				      5)) + (c10 + c11 * temp +
					     c12 * pow (temp,
							2) + c13 * pow (temp,
									3) +
					     c14 * pow (temp,
							4)) * press + (c20 +
								       c21 *
								       temp +
								       c22 *
								       pow
								       (temp,
									2) +
								       c23 *
								       pow
								       (temp,
									3) +
								       c24 *
								       pow
								       (temp,
									4)) *
    pow (press, 2) + (c30 + c31 * temp + c32 * pow (temp, 2)) * pow (press,
								     3);

  SV = Cw + A * sal + B * pow (sal, 1.5) + D * pow (sal, 2);

  return (SV);

}



