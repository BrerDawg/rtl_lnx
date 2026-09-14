#ifndef iir_sos_code_h
#define iir_sos_code_h


/*

================================================================================
iir_sos_code.h
================================================================================

Generic cascaded IIR SOS (biquad) processor.

Uses Direct Form II Transposed implementation.

This module allows a user to create an IIR filter from a vector of
second-order sections (SOS) coefficients.


-------------------------------------------------------------------------------
Coefficient format
-------------------------------------------------------------------------------

Coefficients must be supplied in the following order per section:

    b0 , b1 , b2 , a1 , a2

Note:
    a0 is assumed to be 1.0

Example layout inside vector<float>:

    section0:  b0,b1,b2,a1,a2
    section1:  b0,b1,b2,a1,a2
    section2:  b0,b1,b2,a1,a2


-------------------------------------------------------------------------------
Limits
-------------------------------------------------------------------------------

Maximum sections supported:

    24 SOS sections

Vector size must equal:

    sos_count * 5


-------------------------------------------------------------------------------
Public functions
-------------------------------------------------------------------------------

create( sos_count , vcoeff )

    Loads coefficients and prepares filter.

init()

    Clears internal delay states.

process( x )

    Processes one sample through the SOS cascade.


-------------------------------------------------------------------------------
Return values
-------------------------------------------------------------------------------

create() returns:

    1 = success
    0 = failure

Failures occur if:

    - sos_count == 0
    - sos_count exceeds maximum sections
    - vcoeff size is not multiple of 5
    - vcoeff size does not match sos_count


process()

    If filter not created, returns 0.


-------------------------------------------------------------------------------
Usage example
-------------------------------------------------------------------------------

#include "iir_sos_code.h"

using namespace std;

int main()
{

iir_sos::st_iir_sos_tag filt;

vector<float> vcoeff =
{
0.0009829f,-0.0019542f,0.0009829f,-1.9796481f,0.9798025f,
1.0000000f,-1.9982525f,1.0000000f,-1.9871930f,0.9875462f
};

// or for inbuild coeffs use:  fetch_inbuilt_coeffs( unsigned int which, vector<float> &vcoeff );


if( filt.create(2, vcoeff) == 0 )
    return -1;

float x = 0.5f;

float y = filt.process(x);

}

================================================================================

*/


//iir_sos_code.h
//v1.01        14 mar-2026        //first write up


#include <vector>
#include <stdint.h>

using namespace std;

namespace iir_sos
{


// ------------------------------------------------------------
// configuration
// ------------------------------------------------------------

#define cn_max_sections 24


// ------------------------------------------------------------
// structure
// ------------------------------------------------------------

struct st_iir_sos_tag
{


// ------------------------------------------------------------
// biquad section
// ------------------------------------------------------------

struct biquad
{
    float b0, b1, b2;
    float a1, a2;

    float z0;
    float z1;
};


// ------------------------------------------------------------
// internal storage
// ------------------------------------------------------------

biquad sec[cn_max_sections];

unsigned int sections;

bool created;


// ------------------------------------------------------------
// init()  -- clear delay states
// ------------------------------------------------------------

void init()
{

for(unsigned int i = 0; i < sections; i++)
    {
    sec[i].z0 = 0.0f;
    sec[i].z1 = 0.0f;
    }

}


// ------------------------------------------------------------
// biquad process
// Direct Form II Transposed
// ------------------------------------------------------------

inline float biquad_process( biquad &q, float x )
{

float y = q.b0 * x + q.z0;

q.z0 = q.b1 * x - q.a1 * y + q.z1;

q.z1 = q.b2 * x - q.a2 * y;

return y;

}


// ------------------------------------------------------------
// process()
// ------------------------------------------------------------

inline float process( float x )
{

if( created == 0 )
    return 0.0f;

for(unsigned int i = 0; i < sections; i++)
    {
    x = biquad_process( sec[i], x );
    }

return x;

}


// ------------------------------------------------------------
// create()
// ------------------------------------------------------------

bool create( unsigned int sos_count, vector<float> &vcoeff )
{

created = 0;

if( sos_count == 0 )
    return 0;

if( sos_count > cn_max_sections )
    return 0;

if( (vcoeff.size() % 5) != 0 )
    return 0;

if( (vcoeff.size() / 5) != sos_count )
    return 0;


sections = sos_count;


// load coefficients

unsigned int idx = 0;

for(unsigned int i = 0; i < sos_count; i++)
    {

    sec[i].b0 = vcoeff[idx++];
    sec[i].b1 = vcoeff[idx++];
    sec[i].b2 = vcoeff[idx++];
    sec[i].a1 = vcoeff[idx++];
    sec[i].a2 = vcoeff[idx++];
printf("sec[%d] b0  %f %f %f  %f %f\n", i, sec[i].b0, sec[i].b1, sec[i].b2, sec[i].a1, sec[i].a2 );

    sec[i].z0 = 0.0f;
    sec[i].z1 = 0.0f;

    }

created = 1;

return 1;

}




bool fetch_inbuilt_coeffs( unsigned int which, vector<float> &vcoeff )
{
vcoeff.clear();

switch( which )
	{

	// ----------------------------------------------------------
	// case 0
	// ----------------------------------------------------------

	case 0:
		{
		float sos[5][5] =
		{
			{ 0.0063585f, 0.0116829f, 0.0063585f, -0.9920177f, 0.3011405f },
			{ 1.0000000f, 1.0433289f, 1.0000000f, -0.7367949f, 0.5062629f },
			{ 1.0000000f, 0.4508328f, 1.0000000f, -0.4689320f, 0.7257971f },
			{ 1.0000000f, 0.1651610f, 1.0000000f, -0.3041454f, 0.8726362f },
			{ 1.0000000f, 0.0561941f, 1.0000000f, -0.2354409f, 0.9633122f },
		};

		const unsigned int rows = sizeof(sos)/sizeof(sos[0]);

		for( unsigned int r = 0; r < rows; r++ )
			{
			for( unsigned int c = 0; c < 5; c++ )
				{
				vcoeff.push_back( sos[r][c] );
				}
			}

		return true;
		}



	// ----------------------------------------------------------
	// case 1
	// ----------------------------------------------------------

	case 1:
		{
		float sos[5][5] =
		{
			{ 0.0034367f, 0.0052441f, 0.0034367f, -0.5514932f, 0.0000000f },
			{ 1.0000000f, 1.0000000f, 0.0000000f, -0.9597481f, 0.4059316f },
			{ 1.0000000f, 0.8025740f, 1.0000000f, -0.6745630f, 0.6099674f },
			{ 1.0000000f, 0.3526094f, 1.0000000f, -0.4374853f, 0.7841075f },
			{ 1.0000000f, 0.1325443f, 1.0000000f, -0.2984215f, 0.8971702f },
		};

		const unsigned int rows = sizeof(sos)/sizeof(sos[0]);

		for( unsigned int r = 0; r < rows; r++ )
			{
			for( unsigned int c = 0; c < 5; c++ )
				{
				vcoeff.push_back( sos[r][c] );
				}
			}

		return true;
		}

	}

return false;
}







};


}   // namespace iir_sos

#endif
