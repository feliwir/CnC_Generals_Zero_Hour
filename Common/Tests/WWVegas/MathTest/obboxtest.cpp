/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WWMath                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Tests/mathtest/obboxtest.cpp                 $*
 *                                                                                             *
 *                       Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                     $Modtime:: 12/06/00 9:33a                                              $*
 *                                                                                             *
 *                    $Revision:: 8                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   test_obb_tri -- performs some "hard-coded" obb-tri collision tests                        *
 *   brute_force_cast_obb_tri -- binary-search method of finding the collision time for obb-tr *
 *   brute_force_obb_tri_test -- collide random obb's into random tri's                        *
 *   test_obb_obb -- performs some "hard-coded" obb-obb collision tests                        *
 *   brute_force_cast_obb_obb -- brute force function to verify collision of two obb's         *
 *   brute_force_obb_obb_test -- collide random obb's together                                 *
 *   Test_OBBoxes -- run all of the obb-obb and obb-tri tests                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "vector3.h"
#include "matrix3.h"
#include "tri.h"
#include "obbox.h"
#include "wwmath.h"
#include "colmath.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtest/gtest.h>

//#define MANUAL_DEBUGGING


static Matrix3 _RotateZ45
(
	(float)WWMATH_SQRT2/2.0f,	-(float)WWMATH_SQRT2/2.0f,	0.0f,
	(float)WWMATH_SQRT2/2.0f,	 (float)WWMATH_SQRT2/2.0f,	0.0f,
							  0.0f,								0.0f,	1.0f
);

static Matrix3 _RotateZ90
(
	0.0f, -1.0f, 0.0f,
	1.0f,  0.0f, 0.0f,
	0.0f,  0.0f, 1.0f
);


/*********************************************************************
**
** OBBoxTriTestData
** Test Data for OBBox->Triangle collision
**
*********************************************************************/
class OBBoxTriTestData 
{
public:
	OBBoxClass		Box;
	Vector3			BoxMove;
	Vector3			V0;		
	Vector3			V1;
	Vector3			V2;
	Vector3			N;
	TriClass			Tri;
	float				Fraction;
	bool				StartBad;
	
	OBBoxTriTestData
	(
		const Vector3 &c,					// center of box
		const Vector3 &e,					// extent of box
		const Matrix3 &b,					// basis of box
		const Vector3 &m,					// move for the box
		const Vector3 &v0,				// v0 of triangle
		const Vector3 &v1,				// v1 of triangle
		const Vector3 &v2,				// v2 of triangle
		float frac,							// expected fraction
		bool sol								// expected start solid 
	)
	{
		BoxMove = m;
		V0 = v0;
		V1 = v1;
		V2 = v2;
		Fraction = frac;
		StartBad = sol;

		/*
		** Initialize the triangle
		*/
		Tri.V[0] = &V0;
		Tri.V[1] = &V1;
		Tri.V[2] = &V2;
		Tri.N = & N;
		Tri.Compute_Normal();

		/*
		** Initialize the box
		*/
		Box.Center = c;
		Box.Extent = e;
		Box.Basis = b;
	}
};

static OBBoxTriTestData Test0
(
	Vector3(0,0,0),			// box starting at origin
	Vector3(2,1,1),			// extent is 2 units along x, 1 y, 1 z
	Matrix3(1),
	Vector3(1,0,0),			// moving 5 along x axis
	Vector3(6,-3,-1),			// triangle crossing x and y extent but not colliding
	Vector3(8,-1,2),
	Vector3(9,0,-1),
	1.0f,
	false
);

static OBBoxTriTestData Test1
(
	Vector3(3,0,0),			
	Vector3(1,2,1),
	Matrix3(1),
	Vector3(0,-2,0),
	Vector3(1,-3,0),
	Vector3(2,-3,5),
	Vector3(6,-3,1),
	0.5f,
	false
);

static OBBoxTriTestData Test2
(
	Vector3(-3.5,-1.5,0),	// sweeping a 3x3 box along pos x and neg y
	Vector3(1.5,1.5,1.5),
	Matrix3(1),
	Vector3(4,-4,0),
	Vector3(-4,-4,-1),		// into a polygon in y-z plane
	Vector3(-2,-4,5),
	Vector3(0,-4,1),
	0.25f,						// should only move 25%
	false
);

static OBBoxTriTestData Test3
(
	Vector3(-3.5,-1.5,0) + 0.25f * Vector3(4,-4,0),
	Vector3(1.5,1.5,1.5),	// starting at end of test2's move, should be 0.0 but not StartBad!
	Matrix3(1),
	Vector3(4,-4,0),
	Vector3(-4,-4,-1),		// into a polygon in y-z plane
	Vector3(-2,-4,5),
	Vector3(0,-4,1),
	0.0f,						
	false
);

static OBBoxTriTestData Test4
(
	Vector3(-3.5f,-1.5f,0),	// Same as test 2
	Vector3(1.5f,1.5f,1.5f),
	Matrix3(1),
	Vector3(4,-4,0),
	Vector3(-9,-4,-1),		// into a polygon in y-z plane *but* just barely not in the way
	Vector3(-8,-4,5),
	Vector3(-4.9f,-4,0),
	1.0f,							// should move 100%
	false
);

static OBBoxTriTestData Test5
(
	Vector3(-3.5,-1.5,0),	// Same as test 3 with box brushing polygon vertex.
	Vector3(1.5,1.5,1.5),
	Matrix3(1),
	Vector3(4,-4,0),
	Vector3(-9,-4,-1),		// into a polygon in y-z plane just touching path of box
	Vector3(-8,-4,5),
	Vector3(-4,-4,0),
	1.0f,							// should move 100%
	false
);

static OBBoxTriTestData Test6
(
	Vector3(-3.5f,-1.5f,0),	// Same as test 3 with box brushing polygon vertex.
	Vector3(1.5f,1.5f,1.5f),
	Matrix3(1),
	Vector3(4,-4,0),
	
	Vector3(-9,-4,-1),		// into a polygon in y-z plane just barely hitting it
	Vector3(-8,-4,5),
	Vector3(-3.999f,-4,0),	// (-4,-4,0) would just "touch" (see test5)
	
	0.25f,						// should move 25%
	false
);

static OBBoxTriTestData Test7
(
	Vector3(0,0,0),			// This is a case where the box starts out intersecting
	Vector3(5,5,5),	
	Matrix3(1),
	Vector3(4,4,0),
	Vector3(1,4,-1),
	Vector3(2,4,5),
	Vector3(5,4,0),
	0.0f,					
	true
);

static OBBoxTriTestData Test8
(
	Vector3(-2.5,2,0),		// center
	Vector3(1.5,1,1),			// extent
	Matrix3(1),					// basis
	Vector3(3,0,0),			// move
	
	Vector3(1,2,0),			// v0
	Vector3(3,4,5),			// v1
	Vector3(4,5,-1),			// v2
	0.66666667f,					
	false
);

static OBBoxTriTestData Test9
(
	Vector3(0,0,0),		// Box with diagonal y-z length of 1, rotated 45 about z 
	Vector3(WWMATH_SQRT2/2.0,WWMATH_SQRT2/2.0,1),
	_RotateZ45,
	Vector3(4,0,0),

	Vector3(3,2,-1),		// triangle blocking the move at x=3 (hitting back side)
	Vector3(3,0,1),		
	Vector3(3,-2,-1),

	0.5f,						// hitting another box edge-to-face halfway through the move
	false
);

OBBoxTriTestData* OBBoxTriTestCases[] = 
{
	&Test0,
	&Test1,
	&Test2,
	&Test3,
	&Test4,
	&Test5,
	//&Test6,
	&Test7,
	&Test8,
	&Test9
};

class OBBoxTriTestClass : public ::testing::TestWithParam<OBBoxTriTestData*>
{
protected:
};

/*
** Test OBBox->Triangle collision using the above test data
*/
TEST_P(OBBoxTriTestClass, Collide) {
    CastResultStruct result;
    result.Fraction = 1.0;
    result.StartBad = false;
    result.Normal.Set(0,0,0);

    const OBBoxTriTestData* testcase = GetParam();
	CollisionMath::Collide(	testcase->Box,
							testcase->BoxMove,
							testcase->Tri,
							Vector3(0,0,0),
							&result);

	EXPECT_NEAR(result.Fraction, testcase->Fraction, WWMATH_EPSILON);
    EXPECT_EQ(result.StartBad, testcase->StartBad);
}

INSTANTIATE_TEST_CASE_P(WWMath, OBBoxTriTestClass, ::testing::ValuesIn(OBBoxTriTestCases));

/***********************************************************************************************
 * brute_force_cast_obb_tri -- binary-search method of finding the collision time for obb-tri  *
 *                                                             5                                *
 * This function doesn't really work in the general case.  Only when the endpoint of the move  *
 * is guaranteed to be inside the triangle                                                     *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/8/99     GTH : Created.                                                                 *
 *=============================================================================================*/
float brute_force_cast_obb_tri
(
	const OBBoxClass & box,
	const Vector3 & move,
	const TriClass & tri
)
{
	float istart = 0.0f;
	float iend = 1.0f;
	
	while (iend - istart > WWMATH_EPSILON/2.0f) {
		float icenter = (iend + istart) / 2.0f;
		OBBoxClass testbox = box;
		testbox.Center = box.Center + icenter*move;

		if (Oriented_Box_Intersects_Tri(testbox,tri)) {
			iend = icenter;
		} else {
			istart = icenter;
		}
	}
	return (iend + istart) / 2.0f;
}


/***********************************************************************************************
 * brute_force_obb_tri_test -- collide random obb's into random tri's                          *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/8/99     GTH : Created.                                                                 *
 *=============================================================================================*/
void brute_force_obb_tri_test(int test_count)
{	
	Vector3 v[3];
	Vector3 n;
	OBBoxClass box;
	TriClass tri;
	Vector3 move;

	tri.V[0] = &v[0];
	tri.V[1] = &v[1];
	tri.V[2] = &v[2];
	tri.N = &n;
	
	int fail_count = 0;
	int startbad_count = 0;
	int startbad_fail_count = 0;
	int bad_points = 0;

	float min_fraction = 1.0f;
	float max_fraction = 0.0f;
	float avg_fraction = 0.0f;
	float avg_fraction_error = 0.0f;
	int fraction_error_count = 0;
	float max_error = 0.0f;

	for (int i=0; i<test_count; i++) {

		// v0 is always in positive quadrant
		v[0].X = WWMath::Random_Float(0,5);
		v[0].Y = WWMath::Random_Float(0,5);
		v[0].Z = WWMath::Random_Float(0,5);

		// v1 is always +x,+y,-z
		v[1].X = WWMath::Random_Float(0,5);
		v[1].Y = WWMath::Random_Float(0,5);
		v[1].Z = -WWMath::Random_Float(0,5);

		// v2 is always -x,-y,-z
		v[2].X = -WWMath::Random_Float(0,5);
		v[2].Y = -WWMath::Random_Float(0,5);
		v[2].Z = -WWMath::Random_Float(0,5);

		tri.Compute_Normal();

		// make a random box
		box.Init_Random(0.25f,3.0f);

		// put it in a random position in a 20x20x20 cube
		box.Center.X = WWMath::Random_Float(-10.0f,10.0f);
		box.Center.Y = WWMath::Random_Float(-10.0f,10.0f);
		box.Center.Z = WWMath::Random_Float(-10.0f,10.0f);

		// make a move vector that will move the box's center to the centroid of the tri
		Vector3 new_center;
		new_center.X = (v[0].X + v[1].X + v[2].X) / 3.0f;
		new_center.Y = (v[0].Y + v[1].Y + v[2].Y) / 3.0f;
		new_center.Z = (v[0].Z + v[1].Z + v[2].Z) / 3.0f;
		move = new_center - box.Center;
		
		// sweep box into tri!
		CastResultStruct result;
		result.ComputeContactPoint = true;
		CollisionMath::Collide(box,move,tri,Vector3(0,0,0),&result);

		// if they started out intersecting, this test doesn't count
		if (result.StartBad) {
			startbad_count++;
			if (!Oriented_Box_Intersects_Tri(box,tri)) {
				startbad_fail_count++;
				printf("False startbad!");
			}
		}

		// if they were intersecting, never mind
		if (!result.StartBad) {

			printf(".");
			bool success = true;

			// add the fraction into the total so we get an idea
			// how far our tests are going (hopefully a good mix)
			avg_fraction += result.Fraction;
			if (result.Fraction < min_fraction) min_fraction = result.Fraction;
			if (result.Fraction > max_fraction) max_fraction = result.Fraction;

			// verify that the fraction is correct
			float realfrac = brute_force_cast_obb_tri(box,move,tri);
			if (fabs(realfrac - result.Fraction) > WWMATH_EPSILON) {
				success = false;
				float error = fabs(realfrac - result.Fraction);
				avg_fraction_error += error;
				fraction_error_count++;
				if (error > max_error) max_error = error;
			}

			// verify that they are not intersecting at the end of the move
			// if the allowed move is smaller than epsilon, we skip this and don't move
			if (result.Fraction > WWMATH_EPSILON) {
				OBBoxClass box2 = box;
				CastResultStruct second_result;
				box2.Center += /*0.9999f **/ result.Fraction * move;
				CollisionMath::Collide(box2,move,tri,Vector3(0,0,0),&second_result);

				if (second_result.StartBad) success = false;
				if ((result.Fraction < 1.0f) && (second_result.Fraction > 0.01f)) success = false;
			}

			// if something failed, do the test again to let the programmer step through...
			if (!success) {
				fail_count++;
				CastResultStruct redo_result;
				redo_result.ComputeContactPoint = true;
				CollisionMath::Collide(box,move,tri,Vector3(0,0,0),&redo_result);
			} 
		}			
	}
	printf("\n");
	int passes = test_count - (startbad_fail_count + fail_count);
	printf("Passed %d tests out of %d tests.\n",passes,test_count);
	printf("StartBad tests: %d  failures: %d\n",startbad_count,startbad_fail_count);

	if (fraction_error_count > 0) {
		avg_fraction_error /= (float)fraction_error_count;
	}
	avg_fraction /= (float)(test_count - startbad_count);
	printf("Largest Fraction:     %f\n",max_fraction);
	printf("Smallest Fraction:    %f\n",min_fraction);
	printf("Average Fraction:     %f\n",avg_fraction);
	printf("Average Error:        %f\n",avg_fraction_error);
	printf("Biggest Error:        %f\n",max_error);
}


TEST(WWMath, BruteForceOBBoxTriTest) {
	brute_force_obb_tri_test(1000);
}

/*********************************************************************
**
** OBBoxOBBoxTestData
** Data for testing OBBox->OBBox collision detection
**
*********************************************************************/
class OBBoxOBBoxTestData
{
public:

	OBBoxClass		Box0;
	Vector3			Move0;
	OBBoxClass		Box1;
	Vector3			Move1;
	float				Fraction;
	bool				StartBad;
	
	OBBoxOBBoxTestData
	(
		const Vector3 & c0,		// center of box0
		const Vector3 & e0,		// extent of box0
		const Matrix3 & b0,		// basis of box0
		const Vector3 & m0,		// move for box0

		const Vector3 & c1,		// center of box1
		const Vector3 & e1,		// extent of box1
		const Matrix3 & b1,		// basis of box1
		const Vector3 & m1,		// move for box1

		float frac,					// expected fraction
		bool sol						// expected start solid 
	) :
		Box0(c0,e0,b0),
		Move0(m0),
		Box1(c1,e1,b1),
		Move1(m1),
		Fraction(frac),
		StartBad(sol)
	{
	}
};


static OBBoxOBBoxTestData BTest0
(
	Vector3(0,0,0),		// center
	Vector3(4,0,0),		// extent
	Matrix3(1),				// basis
	Vector3(4,0,0),		// move
	Vector3(6,0,0),		// center
	Vector3(1,1,1),		// extent
	Matrix3(1),				// basis
	Vector3(0,0,0),		// move
	0.25f,					
	false
);

static OBBoxOBBoxTestData BTest1
(
	Vector3(-3.5f,-1.5f,0.0f),
	Vector3(1.5f,1.5f,1.5f),
	Matrix3(1),
	Vector3(4,-4,0),

	Vector3(-5.1,-5,0),		
	Vector3(1,1,1),		
	Matrix3(1),				
	Vector3(0,0,0),		

	1.0f,						// should just barely go by (touches)
	false
);

static OBBoxOBBoxTestData BTest2
(
	Vector3(3,0,0),
	Vector3(7,1,1),
	Matrix3(1),
	Vector3(4,-4,0),

	Vector3(9.5,0,0),		
	Vector3(1,6,1),		
	Matrix3(1),				
	Vector3(0,0,0),		

	0.0f,						// startbad
	true
);

static OBBoxOBBoxTestData BTest3	
(
	Vector3(0,0,0),		// Box with diagonal y-z length of 1, rotated 45 about z 
	Vector3(WWMATH_SQRT2/2.0,WWMATH_SQRT2/2.0,1),
	_RotateZ45,
	Vector3(4,0,0),

	Vector3(4,0,0),		// axis-aligned box blocking the move along the x-axis
	Vector3(1,3,1),		
	Matrix3(1),				
	Vector3(0,0,0),		

	0.5f,						// hitting another box edge-to-face halfway through the move
	false
);

static OBBoxOBBoxTestData BTest4
(
	Vector3(0,0,0),		// Box with diagonal y-z length of 1, rotated 45 about z 
	Vector3(WWMATH_SQRT2/2.0,WWMATH_SQRT2/2.0,1),
	_RotateZ45,
	Vector3(0,4,0),

	Vector3(0,4,0),		// axis-aligned box blocking the move along the x-axis
	Vector3(3,1,1),		
	Matrix3(1),				
	Vector3(0,0,0),		

	0.5f,						// hitting another box edge-to-face halfway through the move
	false
);

static OBBoxOBBoxTestData BTest5
(
	Vector3(0,0,0),		// Box with diagonal y-z length of 1, rotated 45 about z 
	Vector3(WWMATH_SQRT2/2.0,WWMATH_SQRT2/2.0,1),
	_RotateZ45,
	Vector3(0,-4,0),

	Vector3(0,-4,0),		// axis-aligned box blocking the move along the x-axis
	Vector3(3,1,1),		
	Matrix3(1),				
	Vector3(0,0,0),		

	0.5f,						// hitting another box edge-to-face halfway through the move
	false
);

OBBoxOBBoxTestData * OBBoxOBBoxTestCases[] = 
{
	&BTest0,
	&BTest1,
	&BTest2,
	&BTest3,
	&BTest4,
	&BTest5
};

class OBBoxOBBoxTestClass : public ::testing::TestWithParam<OBBoxOBBoxTestData*>
{
protected:
};

/*
** Test OBBox->OBBox collision using the above test data
*/
TEST_P(OBBoxOBBoxTestClass, Collide) {
    CastResultStruct result;
    result.Fraction = 1.0;
    result.StartBad = false;
    result.Normal.Set(0,0,0);

    const OBBoxOBBoxTestData* testcase = GetParam();
    CollisionMath::Collide(testcase->Box0, testcase->Move0, testcase->Box1, testcase->Move1, &result);

    EXPECT_NEAR(result.Fraction, testcase->Fraction, WWMATH_EPSILON);
    EXPECT_EQ(result.StartBad, testcase->StartBad);
}

INSTANTIATE_TEST_CASE_P(WWMath, OBBoxOBBoxTestClass, ::testing::ValuesIn(OBBoxOBBoxTestCases));

/***********************************************************************************************
 * brute_force_cast_obb_obb -- brute force function to verify collision of two obb's           *
 *                                                                                             *
 * This function doesn't really work in the general case.  Only when the endpoint of the move  *
 * is guaranteed to be inside the other box...                                                 *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/8/99     GTH : Created.                                                                 *
 *=============================================================================================*/
float brute_force_cast_obb_obb
(
	const OBBoxClass & box0,
	const Vector3 & move0,
	const OBBoxClass & box1
)
{
	float istart = 0.0f;
	float iend = 1.0f;
	
	while (iend - istart > WWMATH_EPSILON) {
		float icenter = (iend + istart) / 2.0f;
		OBBoxClass testbox = box0;
		testbox.Center = box0.Center + icenter*move0;

		if (Oriented_Boxes_Intersect(testbox,box1)) {
			iend = icenter;
		} else {
			istart = icenter;
		}
	}
	return (iend + istart) / 2.0f;
}


/***********************************************************************************************
 * brute_force_obb_obb_test -- collide random obb's together                                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/8/99     GTH : Created.                                                                 *
 *=============================================================================================*/
void brute_force_obb_obb_test(int test_count)
{	
	OBBoxClass box0;
	OBBoxClass box1;
	Vector3 move0;

	int fail_count = 0;
	int startbad_count = 0;
	int startbad_fail_count = 0;

	float avg_fraction = 0.0f;
	float avg_fraction_error = 0.0f;
	int fraction_error_count = 0;
	float max_error = 0.0f;
	float min_fraction = 1.0f;
	float max_fraction = 0.0f;

	for (int i=0; i<test_count; i++) {

		box0.Init_Random(0.25f,3.0f);
		box1.Init_Random(0.25f,3.0f);
		
		box0.Center.X = WWMath::Random_Float(-10.0f,10.0f);
		box0.Center.Y = WWMath::Random_Float(-10.0f,10.0f);
		box0.Center.Z = WWMath::Random_Float(-10.0f,10.0f);
		box1.Center.Set(0,0,0);

		Vector3 newcenter;
		newcenter.X = WWMath::Random_Float(-0.12f,0.12f);	// new center must be inside other box
		newcenter.Y = WWMath::Random_Float(-0.12f,0.12f);
		newcenter.Z = WWMath::Random_Float(-0.12f,0.12f);
		move0 = newcenter - box0.Center;

		// sweep box0 into box1
		CastResultStruct result;
		result.ComputeContactPoint = true;
		CollisionMath::Collide(box0,move0,box1,Vector3(0,0,0),&result);

		// if they were intersecting, verify that
		if (result.StartBad) {
			startbad_count++;
			if (!Oriented_Boxes_Intersect(box0,box1)) {
				startbad_fail_count++;
				printf("False startbad!");
			}
		}

		// if they weren't intersecting, verify the allowed move
		if (!result.StartBad) {

			bool success = true;

			// add the fraction into the total so we get an idea
			// how far our tests are going (hopefully a good mix)
			avg_fraction += result.Fraction;
			if (result.Fraction < min_fraction) min_fraction = result.Fraction;
			if (result.Fraction > max_fraction) max_fraction = result.Fraction;

			// verify that the fraction is correct
			float realfrac = brute_force_cast_obb_obb(box0,move0,box1);
			if (fabs(realfrac - result.Fraction) > WWMATH_EPSILON) {
				success = false;
				float error = fabs(realfrac - result.Fraction);
				avg_fraction_error += error;
				fraction_error_count++;
				if (error > max_error) max_error = error;
			}
			
			// verify that they are not intersecting now
			// if the allowed move is smaller than epsilon, we skip this and don't move
			if (result.Fraction > WWMATH_EPSILON) {

				OBBoxClass box2 = box0;
				box2.Center += (1.0f - WWMATH_EPSILON) * result.Fraction * move0;

				CastResultStruct second_result;
				CollisionMath::Collide(box2,move0,box1,Vector3(0,0,0),&second_result);

				if (second_result.StartBad) {
					success = false;
#ifdef MANUAL_DEBUGGING
					_asm int 0x03;
					while (!success) {
						CastResultStruct move_result;
						CollisionMath::Collide(box0,move0,box1,Vector3(0,0,0),&move_result);
						second_result.Reset();
						CollisionMath::Collide(box2,move0,box1,Vector3(0,0,0),&second_result);
					}
#endif
				}
				if ((result.Fraction < 1.0f) && (second_result.Fraction > 0.01f)) success = false;
			}

			// if something failed, do the test again to let the programmer step through...
			if (!success) {
#ifdef MANUAL_DEBUGGING
				_asm int 0x03;
				while (!success) {
					fail_count++;
					CastResultStruct redo_result;
					redo_result.ComputePoint = true;
					CollisionMath::Collide(box0,move0,box1,Vector3(0,0,0),&redo_result);
				}			
#endif
				printf("x");
			} else {
				printf(".");
			}
		}			
	}
	printf("\n");

	int passes = test_count - (startbad_fail_count + fail_count);
	printf("Passed %d tests out of %d tests.\n",passes,test_count);
	printf("StartBad tests: %d  failures: %d\n",startbad_count,startbad_fail_count);

	if (fraction_error_count) {
		avg_fraction_error /= (float)fraction_error_count;
	}
	avg_fraction /= (float)(test_count - startbad_count);

	printf("Largest Fraction:   %f\n",max_fraction);
	printf("Smallest Fraction:  %f\n",min_fraction);
	printf("Average Fraction:   %f\n",avg_fraction);
	printf("Average Error:      %f\n",avg_fraction_error);
	printf("Biggest Error:      %f\n",max_error);
}

TEST(WWMath, BruteForceOBBoxOBBoxTest) {
	brute_force_obb_obb_test(1000);
}
