/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Wilson Yeh
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glm/glm.hpp>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 540

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

// margin of error for floating point operations
#define EPSILON 0.01

// How fine the supersampling is. Square this number for the number of subpixels per pixel
#define SUPER_SAMPLE 3

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

double sphere_intersection(unsigned int sphereInd, glm::vec3 origin, glm::vec3 ray) {
	glm::vec3 spherepos(spheres[sphereInd].position[0], spheres[sphereInd].position[1], spheres[sphereInd].position[2]);
	double radius = spheres[sphereInd].radius;
	double b = 2 * glm::dot(ray, origin - spherepos);
	double c = glm::dot(origin - spherepos, origin - spherepos) - radius*radius;

	double term = b * b - 4 * c;
	if (term < 0) {
		return -1;
	}
	double sqroot = sqrt(term);
	double negroot = (-b - sqroot) / 2;
	if (negroot > 0) {
		return negroot;
	}
	else {
		double posroot = (-b + sqroot) / 2;
		return posroot;
	}
}

// For intersection point, returns t and barycentric coordinates
// TODO: switch to smart ptr later
double* triangle_intersection(unsigned int triangleInd, glm::vec3 origin, glm::vec3 ray) {
	// Find intersection point of ray and plane of triangle
	Vertex vert0 = triangles[triangleInd].v[0];
	Vertex vert1 = triangles[triangleInd].v[1];
	Vertex vert2 = triangles[triangleInd].v[2];
	glm::vec3 v0(vert0.position[0], vert0.position[1], vert0.position[2]);
	glm::vec3 v1(vert1.position[0], vert1.position[1], vert1.position[2]);
	glm::vec3 v2(vert2.position[0], vert2.position[1], vert2.position[2]);
	glm::vec3 side0 = v1 - v0;
	glm::vec3 side1 = v2 - v0;
	glm::vec3 planeNormal = glm::normalize(glm::cross(side0, side1));
	
	double denom = glm::dot(planeNormal, ray);
	if (denom >= -EPSILON && denom <= EPSILON) { // almost 0, ray parallel to plane
		return new double[4]{ -1, 0, 0, 0 };
	}

	double D = glm::dot(planeNormal, v0);

	// triangle normals always point towards camera in positive z direction
	// primary rays point in negative z direction, resulting in negative dot product
	// check sign against ray orientation
	double sign = (ray[2] < 0) ? 1.0 : -1.0;
	double t = (sign * glm::dot(planeNormal, origin) + D) / denom;

	if (t < 0) {
		return new double[4]{ t, 0, 0, 0 };
	}

	// Check if intersection point is in triangle
	glm::vec3 intersection = origin + (ray * (float)t);
	double iprojx;
	double iprojy;
	double v0projx;
	double v0projy;
	double v1projx;
	double v1projy;
	double v2projx;
	double v2projy;

	// Project triangle and intersection
	if (glm::dot(glm::vec3(1, 0, 0), planeNormal) != 0) {
		// Project onto x=0 (normals are not perpendicular)
		iprojx = intersection[1];
		iprojy = intersection[2];
		v0projx = v0[1];
		v0projy = v0[2];
		v1projx = v1[1];
		v1projy = v1[2];
		v2projx = v2[1];
		v2projy = v2[2];
	}
	else if (glm::dot(glm::vec3(0, 1, 0), planeNormal) != 0) {
		// Project onto y=0 (normals are not perpendicular)
		iprojx = intersection[0];
		iprojy = intersection[2];
		v0projx = v0[0];
		v0projy = v0[2];
		v1projx = v1[0];
		v1projy = v1[2];
		v2projx = v2[0];
		v2projy = v2[2];
	}
	else if (glm::dot(glm::vec3(0, 0, 1), planeNormal) != 0) {
		// Project onto z=0 (normals are not perpendicular)
		iprojx = intersection[0];
		iprojy = intersection[1];
		v0projx = v0[0];
		v0projy = v0[1];
		v1projx = v1[0];
		v1projy = v1[1];
		v2projx = v2[0];
		v2projy = v2[1];
	}

	double triangleArea = 0.5 * ((v1projx - v0projx) * (v2projy - v0projy) - (v2projx - v0projx) * (v1projy - v0projy));
	double alpha = 0.5 / triangleArea * ((v1projx - iprojx) * (v2projy - iprojy) - (v2projx - iprojx) * (v1projy - iprojy));
	double beta = 0.5 / triangleArea * ((iprojx - v0projx) * (v2projy - v0projy) - (v2projx - v0projx) * (iprojy - v0projy));
	double gamma = 0.5 / triangleArea * ((v1projx - v0projx) * (iprojy - v0projy) - (iprojx - v0projx) * (v1projy - v0projy));

	if (alpha >= 0 && beta >= 0 && gamma >= 0 && alpha <= 1 && beta <= 1 && gamma <= 1 &&
		1 - alpha - beta - gamma >= -EPSILON && 1 - alpha - beta - gamma <= EPSILON) { // +-EPSILON error
		// Intersection point is inside triangle
		return new double[4]{ t, alpha, beta, gamma };
	}
	else {
		// Intersection point is outside triangle
		return new double[4]{ -5, 0, 0, 0 };
	}
}

//MODIFY THIS FUNCTION
void draw_scene()
{
	unsigned char super_sample_buffer[SUPER_SAMPLE][HEIGHT*SUPER_SAMPLE][3];

	double fovRad = fov * 3.14159265 / 180;

	double imageZ = -1;
	double imageY = tan(fovRad / 2);
	double imageX = (1.0 * WIDTH / HEIGHT) * imageY;

	double halfWidth = WIDTH / 2 * SUPER_SAMPLE;
	double halfHeight = HEIGHT / 2 * SUPER_SAMPLE;

	for(unsigned int x=0; x<WIDTH*SUPER_SAMPLE; x++)
	{
		if (x > 0 && x % SUPER_SAMPLE == 0) {
			glPointSize(2.0);
			glBegin(GL_POINTS);

			for (int ypixel = 0; ypixel < HEIGHT; ++ypixel) {
				unsigned int avgr = 0;
				unsigned int avgg = 0;
				unsigned int avgb = 0;
				for (int i = 0; i < SUPER_SAMPLE; ++i) {
					for (int j = ypixel * SUPER_SAMPLE; j < ypixel * SUPER_SAMPLE + SUPER_SAMPLE; ++j) {
						avgr += super_sample_buffer[i][j][0];
						avgg += super_sample_buffer[i][j][1];
						avgb += super_sample_buffer[i][j][2];
					}
				}
				plot_pixel(x / 3 - 1, ypixel,
					static_cast<unsigned char>(avgr / SUPER_SAMPLE / SUPER_SAMPLE),
					static_cast<unsigned char>(avgg / SUPER_SAMPLE / SUPER_SAMPLE),
					static_cast<unsigned char>(avgb / SUPER_SAMPLE / SUPER_SAMPLE));
			}

			glEnd();
			glFlush();
		}

		// Shoot ray from (0,0,0) camera to image plane
		double rayX = (x - halfWidth) / halfWidth * imageX; // Proportionally close to left/right side of image plane
		for(unsigned int y=0; y<HEIGHT*SUPER_SAMPLE; y++)
		{
			double rayY = (y - halfHeight) / halfHeight * imageY; // Proportionally close to top/bottom side of image plane
			glm::vec3 ray = glm::normalize(glm::vec3(rayX, rayY, imageZ));

			// Find closest intersection
			double tmin = -1.0;
			bool isSphere = false;
			int objectInd = -1;

			for (int i = 0; i < num_spheres; ++i) {
				double t = sphere_intersection(i, glm::vec3(0,0,0), ray);
				if (t > EPSILON && (tmin < 0 || t < tmin)) {
					tmin = t;
					isSphere = true;
					objectInd = i;
				}
			}

			// Save barycentric coordinates if triangle intersection
			double barycentric_coords[3];
			for (int i = 0; i < num_triangles; ++i) {
				double* intersection_data = triangle_intersection(i, glm::vec3(0,0,0), ray);
				double t = intersection_data[0];
				if (t > EPSILON && (tmin < 0 || t < tmin)) {
					tmin = t;
					isSphere = false;
					objectInd = i;
					barycentric_coords[0] = intersection_data[1];
					barycentric_coords[1] = intersection_data[2];
					barycentric_coords[2] = intersection_data[3];
				}
				delete[] intersection_data; // switch to smart ptr later
			}

			if (tmin > 0) {
				// Add contribution from each light to the intersection point
				glm::vec3 intersectPoint = ray * (float)tmin;
				double rgb[3];
				for (int i = 0; i < 3; ++i) {
					rgb[i] = 0;
				}

				glm::vec3 pointNormal;
				double pointDiffuse[3];
				double pointSpecular[3];
				double pointShininess;
				if (isSphere) {
					// Calculate point normal and properties of sphere
					Sphere sphere = spheres[objectInd];
					pointNormal = intersectPoint - glm::vec3(sphere.position[0], sphere.position[1], sphere.position[2]);
					pointNormal /= sphere.radius; // normalize

					for (int j = 0; j < 3; ++j) {
						pointDiffuse[j] = sphere.color_diffuse[j];
						pointSpecular[j] = sphere.color_specular[j];
					}
					pointShininess = spheres[objectInd].shininess;
				}
				else {
					// Calculate interpolated normal and coefficients of triangle
					pointNormal = glm::vec3(0, 0, 0);
					for (int j = 0; j < 3; ++j) {
						pointDiffuse[j] = 0.0;
						pointSpecular[j] = 0.0;
					}
					pointShininess = 0.0;
					for (int i = 0; i < 3; ++i) {
						Vertex v = triangles[objectInd].v[i];
						glm::vec3 normal(v.normal[0], v.normal[1], v.normal[2]);
						pointNormal += glm::normalize(normal) * (float)barycentric_coords[i];

						for (int j = 0; j < 3; ++j) {
							pointDiffuse[j] += v.color_diffuse[j] * (float)barycentric_coords[i];
							pointSpecular[j] += v.color_specular[j] * (float)barycentric_coords[i];
						}
						pointShininess += v.shininess * (float)barycentric_coords[i];
					}
				}

				for (int i = 0; i < num_lights; ++i) {
					// Check if the light is blocked (object in shadow)
					bool inShadow = false;
					glm::vec3 lightPos(lights[i].position[0], lights[i].position[1], lights[i].position[2]);
					glm::vec3 shadowRay = lightPos - intersectPoint;
					double len = sqrt(glm::dot(shadowRay, shadowRay));
					if (len <= 0) {
						continue;
					}
					shadowRay /= (float)len; // normalize
					double lightT = len + EPSILON; // t-steps from intersection to light

					for (int j = 0; j < num_spheres && !inShadow; ++j) {
						double t = sphere_intersection(j, intersectPoint, shadowRay);
						if (t > EPSILON && t < lightT) {
							// There is a sphere between the intersection point and the light source
							inShadow = true;
						}
					}
					for (int j = 0; j < num_triangles && !inShadow; ++j) {
						double* intersection_data = triangle_intersection(j, intersectPoint, shadowRay);
						double t = intersection_data[0];
						if (t > EPSILON && t < lightT) {
							// There is a triangle between the intersection point and the light source
							inShadow = true;
						}
						delete[] intersection_data; // switch to smart ptr later
					}

					if (!inShadow) {
						// Add light's contribution
						double ldotn = glm::dot(shadowRay, pointNormal);
						if (ldotn < 0) {
							ldotn = 0;
						}
						double rdotv = glm::dot(-shadowRay - (2*glm::dot(-shadowRay, pointNormal) * pointNormal), -ray);
						if (rdotv < 0) {
							rdotv = 0;
						}
						for (int j = 0; j < 3; ++j) {
							rgb[j] += lights[i].color[j] * (pointDiffuse[j] * ldotn + pointSpecular[j] * pow(rdotv, pointShininess));
						}
					}
				}

				// Add ambient light, clamp any blown out values
				for (int i = 0; i < 3; ++i) {
					rgb[i] += ambient_light[i];
					if (rgb[i] > 1.0) { rgb[i] = 1.0; }
					super_sample_buffer[x % 3][y][i] = static_cast<unsigned char>(rgb[i] * 255);
				}
			}
			else {
				// No intersection, white background
				for (int i = 0; i < 3; ++i) {
					super_sample_buffer[x % 3][y][i] = static_cast<unsigned char>(255);
				}
			}
		}
	}
	glPointSize(2.0);
	glBegin(GL_POINTS);

	for (int ypixel = 0; ypixel < HEIGHT; ++ypixel) {
		unsigned int avgr = 0;
		unsigned int avgg = 0;
		unsigned int avgb = 0;
		for (int i = 0; i < SUPER_SAMPLE; ++i) {
			for (int j = ypixel * SUPER_SAMPLE; j < ypixel * SUPER_SAMPLE + SUPER_SAMPLE; ++j) {
				avgr += super_sample_buffer[i][j][0];
				avgg += super_sample_buffer[i][j][1];
				avgb += super_sample_buffer[i][j][2];
			}
		}
		plot_pixel(WIDTH - 1, ypixel,
			static_cast<unsigned char>(avgr / SUPER_SAMPLE),
			static_cast<unsigned char>(avgg / SUPER_SAMPLE),
			static_cast<unsigned char>(avgb / SUPER_SAMPLE));
	}

	glEnd();
	glFlush();
	printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
	  parse_doubles(file, "pos:", l.position);
	  parse_doubles(file, "col:", l.color);

	  // Multiply into 27 lights in a cube for soft shadows
	  l.color[0] /= 27;
	  l.color[1] /= 27;
	  l.color[2] /= 27;
	  for (int i = -1; i < 2; ++i) {
		  for (int j = -1; j < 2; ++j) {
			  for (int k = -1; k < 2; ++k) {
				  if (num_lights == MAX_LIGHTS)
				  {
					  printf("too many lights, you should increase MAX_LIGHTS!\n");
					  exit(0);
				  }
				  Light templ;
				  templ.color[0] = l.color[0];
				  templ.color[1] = l.color[1];
				  templ.color[2] = l.color[2];
				  templ.position[0] = l.position[0] + 0.05 * i;
				  templ.position[1] = l.position[1] + 0.05 * j;
				  templ.position[2] = l.position[2] + 0.05 * k;
				  lights[num_lights++] = templ;
			  }
		  }
	  }
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}

