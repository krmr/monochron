/** ***********************************************************************
Title:		AVR-SUN
Author:		Uwe Nagel
Date:		2003 - 2008
Compiler:	AVR-GCC
Hardware:	Any AVR with enough memory
 @defgroup ulegan_sun Sonnenauf- und untergangsberechnung
 @brief berechnet Sonnenauf- und untergang in Abhängigkeit von Datum und
 @brief geografischer Position
**************************************************************************/
/*@{*/


#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "ratt.h"

/*
** constant definitions
*/

#define Pi M_PI
#define ZPi 2*M_PI
#define PY 0.0174532925199432957692369076848861
#define Pi2 1.5707963267948966192313216916398
#define frac(x) ((x)-floor(x))
#define int(x)  ((x)<0? -ceil(-x):floor(x))

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t date_m, date_d, date_y;

// variables for AutoDim from AdvancedFeatures.c
extern volatile uint16_t autodim_day_time;
extern volatile uint16_t autodim_night_time;


#define LAT_RAD 48.053533*PY
#define LNG_HOUR 10.881767/15
#define LOCAL_OFFSET 2

uint16_t getSun(uint8_t rise){

/*!
 * Source:
 *	Almanac for Computers, 1990
 *	published by Nautical Almanac Office
 *	United States Naval Observatory
 *	Washington, DC 20392
 *
 * http://www.best.com/~williams/sunrise_sunset_algorithm.htm
 *
 * Inputs:
 * - datum:      date of sunrise/sunset
 * - koord:   location for sunrise/sunset
 * - zenith:                Sun's zenith for sunrise/sunset defined as macro
 * 	- offical      = 90 degrees 50'  == Aufgang
 * 	- civil        = 96 degrees
 * 	- nautical     = 102 degrees   == Daemmerung
 * 	- astronomical = 108 degrees
 *
 * - rise: 1 for calculating sunrise, 0 for calculating sunset
 * - localOffset: offset to UTC. 1 for MEZ, 2 for MESZ
 * Output:
 * - zeit: structure for rise/set-time
 *	NOTE: longitude is positive for East and negative for West

 * Beispiel: Berechnet die Auf- und Untergangszeiten, wie sie im 'Himmelsjahr' angegeben sind
 * =========
date_t datum;
lonlat_t koord;
time_t rise_time;
time_t fall_time;
	koord.longitude=10.0;
	koord.latitude=50.0;
	datum.year=2008;
	datum.month=1;
	datum.day=19;
	sun(datum,koord,1,1,&rise_time);
	sun(datum,koord,0,1,&fall_time);


 */

#define zenith 90.833333333333

uint16_t N1, N3, N;
float lngHour, t;
float M, L, RA;
float SinDec, CosDec, CosH;
float H, T, UT, localT;

/* 1. first calculate the day of the year */

	N1 = (275 * date_m) / 9;
//0-1 1-2 2-2 3-2
	N3 = (date_y & 3) ? 2:1;
	N = N1 +date_d - 30;
	if( date_m>2 ) N-=N3;

/* 2. convert the longitude to hour value and calculate an approximate time */

	lngHour = LNG_HOUR;

	t = rise ? 6:18;
	t = N + ((t - lngHour) / 24);

/* 3. calculate the Sun's mean anomaly */

	M = (0.017201965 * t) - 0.057403879;

/* 4. calculate the Sun's true longitude */

	L = M + ((0.033440508 * sin(M)) + (0.0003490658504 * sin(2 * M)) + 4.932893878);
	if( L>=ZPi ) L-=ZPi;
	if( L<0 )    L+=ZPi;	// NOTE: L potentially needs to be adjusted into the range [0,360) by adding/subtracting 360

/* 5a. calculate the Sun's right ascension */

	RA = atan(0.91764 * tan(L));
	if( RA>ZPi ) RA-=ZPi;
	if( RA<0 )   RA+=ZPi;	// NOTE: RA potentially needs to be adjusted into the range [0,360) by adding/subtracting 360

/* 5b. right ascension value needs to be in the same quadrant as L */

	RA = RA/PY + ( (floor( L/Pi2)) - (floor(RA/Pi2)) )* 90.0;

/* 5c. right ascension value needs to be converted into hours */

	RA /= 15;

/* 6. calculate the Sun's declination */

	SinDec = 0.39782 * sin(L);
	CosDec = cos(asin(SinDec));

/* 7a. calculate the Sun's local hour angle */

	CosH = (cos(zenith*PY) - (SinDec * sin(LAT_RAD))) / (CosDec * cos(LAT_RAD));

//	if (CosH >  1) document.write("Die Sonne geht hier heute nicht auf");
//	if (CosH < -1) document.write("Die Sonne geht hier heute nicht unter");

/* 7b. finish calculating H and convert into hours */

	H = acos(CosH)/PY;
	if( rise )  H = 360.0 - H;
	H /= 15.0;

/* 8. calculate local mean time of rising/setting */
	T = H + RA - (0.06571 * t) - 6.622;

/* 9. adjust back to UTC */

	UT = T - lngHour;
	if( UT<0 )		UT += 24;
	if( UT>=24 ) 	UT -= 24;	// NOTE: UT potentially needs to be adjusted into the range [0,24) by adding/subtracting 24

/* 10. convert UT value to local time zone of latitude/longitude */

	localT = UT + LOCAL_OFFSET;

	return int(60.0*localT);
}

void setSun(void) {
//	set times to switch dimming on and off to sunrise and sunset
	autodim_day_time = getSun(1);
	autodim_night_time = getSun(0);
}
/*@}*/
