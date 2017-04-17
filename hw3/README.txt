Assignment #3: Ray tracing

FULL NAME: Wilson Yeh
EMAIL: wilsonye@usc.edu
ID: 4780838615


MANDATORY FEATURES
------------------

<Under "Status" please indicate whether it has been implemented and is
functioning correctly.  If not, please explain the current status.>

Feature:                                 Status: finish? (yes/no)
-------------------------------------    -------------------------
1) Ray tracing triangles                  yes (some artifacts on complex scenes with lots of triangles)

2) Ray tracing sphere                     yes

3) Triangle Phong Shading                 yes

4) Sphere Phong Shading                   yes

5) Shadows rays                           yes

6) Still images                           yes
   
7) Extra Credit (up to 20 points)
	1. Soft shadows (10 points)
		- Done by multiplying each light into a cube of 27 lights about .05 apart, simulating an area light
	2. Good antialiasing (10 points)
		- Done by direct supersampling
		- Split each pixel into 9 subpixels
		- Average the 9 subpixels to get the antialiased color of the actual pixel

8) Other

Scene description files used:
	000 test1.scene
	001 test2.scene
	002 spheres.scene
	003 table.scene

If rendering takes too long, you can make the supersampling less fine by changing:

	#define SUPER_SAMPLE 3
	to
	#define SUPER_SAMPLE 2

Otherwise probably need some optimization flags
