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
#define MAX_LIGHTS 100

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

double sphere_intersection(unsigned int sphereInd, glm::vec3 ray) {
	glm::vec3 spherepos(spheres[sphereInd].position[0], spheres[sphereInd].position[1], spheres[sphereInd].position[2]);
	double radius = spheres[sphereInd].radius;
	double b = 2 * glm::dot(ray, -spherepos);
	double c = glm::dot(spherepos, spherepos) - radius*radius;

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

double triangle_intersection(unsigned int triangleInd, glm::vec3 ray) {
	// Find intersection point of ray and plane of triangle
	Vertex vert0 = triangles[triangleInd].v[0];
	Vertex vert1 = triangles[triangleInd].v[1];
	Vertex vert2 = triangles[triangleInd].v[2];
	glm::vec3 v0(vert0.position[0], vert0.position[1], vert0.position[2]);
	glm::vec3 v1(vert1.position[0], vert1.position[1], vert1.position[2]);
	glm::vec3 v2(vert2.position[0], vert2.position[1], vert2.position[2]);
	glm::vec3 side0 = v0 - v1;
	glm::vec3 side1 = v0 - v2;
	glm::vec3 planeNormal = glm::normalize(glm::cross(side0, side1));

	double D = glm::dot(planeNormal, v0);
	double denom = glm::dot(planeNormal, ray);
	if (denom == 0) {
		return -1;
	}

	double t = D / denom;
	if (t < 0) {
		return t;
	}

	// Check if intersection point is in triangle
	glm::vec3 intersection = ray * (float)t;
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
		1 - alpha - beta - gamma >= -0.0001 && 1 - alpha - beta - gamma <= 0.0001) { // +-0.0001 error
		// Intersection point is inside triangle
		return t;
	}
	else {
		// Intersection point is outside triangle
		return -1;
	}
}

//MODIFY THIS FUNCTION
void draw_scene()
{
	double fovRad = fov * 3.14159265 / 180;

	double halfWidth = WIDTH / 2;
	double halfHeight = HEIGHT / 2;

	double imageZ = -1;
	double imageY = tan(fovRad / 2);
	double imageX = (1.0 * WIDTH / HEIGHT) * imageY;

	for(unsigned int x=0; x<WIDTH; x++)
	{
		glPointSize(2.0);  
		glBegin(GL_POINTS);

		// Shoot ray from (0,0,0) camera to image plane
		double rayX = (x - halfWidth) / halfWidth * imageX; // Proportionally close to left/right side of image plane
		for(unsigned int y=0; y<HEIGHT; y++)
		{
			double rayY = (y - halfHeight) / halfHeight * imageY; // Proportionally close to top/bottom side of image plane
			glm::vec3 ray = glm::normalize(glm::vec3(rayX, rayY, imageZ));

			// Calculate minimum distance to a sphere
			double tmin = -1.0;

			for (int i = 0; i < num_spheres; ++i) {
				double t = sphere_intersection(i, ray);
				if (t > 0 && (tmin < 0 || t < tmin)) {
					tmin = t;
				}
			}

			for (int i = 0; i < num_triangles; ++i) {
				double t = triangle_intersection(i, ray);
				if (t > 0 && (tmin < 0 || t < tmin)) {
					tmin = t;
				}
			}
			
			if (tmin > 0) {
				plot_pixel(x, y, 0, 0, 0);
			}
			else {
				plot_pixel(x, y, 255, 255, 255);
			}
		}
		glEnd();
		glFlush();
	}
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
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
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

