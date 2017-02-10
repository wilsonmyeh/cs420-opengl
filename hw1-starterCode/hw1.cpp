/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: <wilsonye>
*/

#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

// BEGIN ADDITIONS
GLuint buffer;
OpenGLMatrix *matrix;

// Shader and uniform variables stuff
BasicPipelineProgram *pipelineProgram;
GLint program;
GLint h_modelViewMatrix, h_projectionMatrix;
GLuint vao;

// Quad Stuff
//GLfloat theta[3] = { 0.0, 0.0, 0.0 };
//GLfloat delta = 2.0;
//GLint axis = 2;
//GLint spin = 1;

// Triangle specifications
int numVertices = 3;
float positions[9] =
{0, 0, -1,
 1, 0, -1,
 0, 1, -1 };
float colors[12] =
{1, 0, 0, 1,	// Red
 0, 1, 0, 1,	// Green
 0, 0, 1, 1 };	// Blue
// END ADDITIONS

ImageIO * heightmapImage;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

//void spinQuad()
//{
//	// spin the quad delta degrees around the selected axis
//	if (spin)
//		theta[axis] += delta;
//	if (theta[axis] > 360.0)
//		theta[axis] -= 360.0;
//	// display result (do not forget this!)
//	glutPostRedisplay();
//}

//void renderQuad()
//{
//	GLint first = 0;
//	GLsizei numberOfVertices = 6;
//	glDrawArrays(GL_TRIANGLES, first, numberOfVertices);
//}

void bindProgram()
{
	// bind our buffer, so that glVertexAttribPointer refers
	// to the correct buffer
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	const void * offset = (const void*)0;
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
	GLuint loc2 = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc2);
	offset = (const void*) sizeof(positions);
	glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset);

	// write projection and modelview matrix to shader
	GLboolean isRowMajor = GL_FALSE;

	float m[16]; // column-major
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	matrix->GetMatrix(m);
	// upload m to the GPU
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);

	float p[16]; // column-major
	matrix->SetMatrixMode(OpenGLMatrix::Projection);
	matrix->GetMatrix(p);
	// upload p to the GPU
	glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

void displayFunc()
{
  // render some stuff...
	glClear(GL_COLOR_BUFFER_BIT |
		GL_DEPTH_BUFFER_BIT);
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	matrix->LoadIdentity();
	double zStudent = 3 + (4780838615.0 / 10000000000.0);
	matrix->LookAt(0, 0, zStudent, 0, 0, -1, 0, 1, 0);

	bindProgram();

	pipelineProgram->Bind(); // bind the pipeline program
	glBindVertexArray(vao); // bind the VAO

	GLint first = 0;
	GLsizei count = numVertices;
	glDrawArrays(GL_TRIANGLES, first, count);

	glBindVertexArray(0); // unbind the VAO

	//matrix->Rotate(theta[0], 1.0, 0.0, 0.0);
	//matrix->Rotate(theta[1], 0.0, 1.0, 0.0);
	//matrix->Rotate(theta[2], 0.0, 0.0, 1.0);
	//renderQuad();

	glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff... 

  // for example, here, you can save the screenshots to disk (to make the animation)

  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  GLfloat aspect = (GLfloat)w / (GLfloat)h;
  glViewport(0, 0, w, h);

  // setup perspective matrix...
  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  matrix->LoadIdentity();
  matrix->Perspective(45.0f, 1280.0f/720.0f, 0.01f, 1000.0f); // Hard coded aspect ratio for milestone
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  matrix->LoadIdentity();
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
  }
}

void initVAO()
{
	// create the vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // bind the VAO

	// bind the VBO “buffer” (must be previously created)
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	// get location index of the “position” shader variable
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0; GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
	offset = (const void*) sizeof(positions);
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO
}

void initPipelineProgram()
{
	pipelineProgram = new BasicPipelineProgram();
	pipelineProgram->Init("../openGLHelper-starterCode");
	pipelineProgram->Bind(); // must do (once) before glUniformMatrix4fv

	// get a handle to the program
	program = pipelineProgram->GetProgramHandle();

	// initialize uniform variable handles
	// get a handle to the modelViewMatrix shader variable
	h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");

	// do the same for the projectionMatrix
	h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");

	initVAO();
}

void initVBO()
{
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(colors),
		NULL, GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data to it

	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		sizeof(positions), positions);

	// upload color data
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions),
		sizeof(colors), colors);
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  matrix = new OpenGLMatrix();
  initVBO();
  initPipelineProgram();
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


