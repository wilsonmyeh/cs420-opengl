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
char renderType;
OpenGLMatrix *matrix;

GLuint buffer;
GLuint linesBuffer;
GLuint trianglesBuffer;

// Shader and uniform variables
BasicPipelineProgram *pipelineProgram;
BasicPipelineProgram *linesPipelineProgram;
BasicPipelineProgram *trianglesPipelineProgram;

GLint program;
GLint linesProgram;
GLint trianglesProgram;

GLint h_modelViewMatrix, h_projectionMatrix;
GLint lines_h_modelViewMatrix, lines_h_projectionMatrix;
GLint triangles_h_modelViewMatrix, triangles_h_projectionMatrix;

GLuint vao;
GLuint linesvao;
GLuint trianglesvao;

// Quad Stuff
//GLfloat theta[3] = { 0.0, 0.0, 0.0 };
//GLfloat delta = 2.0;
//GLint axis = 2;
//GLint spin = 1;

// Vertices
int numVertices;
float* positions;
float* colors;
int positionSize;
int colorSize;

int numLineVertices;
float* linePositions;
float* lineColors;
int linePositionSize;
int lineColorSize;

int numTriVertices;
float* triPositions;
float* triColors;
int triPositionSize;
int triColorSize;
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

void displayFunc()
{
  // render some stuff...
	glClear(GL_COLOR_BUFFER_BIT |
		GL_DEPTH_BUFFER_BIT);

	// Compute the ModelView matrix
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	matrix->LoadIdentity();
	matrix->LookAt( heightmapImage->getHeight()*2.0f, heightmapImage->getHeight()*2.0f, heightmapImage->getWidth()*-3.0f,
					heightmapImage->getHeight()*0.5f, 0.0f, heightmapImage->getWidth()*-0.5f, 0.0f, 1.0f, 0.0f);

	GLint typeModelViewMatrix;
	GLint typeProjectionMatrix;
	BasicPipelineProgram *typePipelineProgram;
	GLuint typevao;
	int typeNumVertices;
	GLuint drawType;

	switch (renderType)
	{
	case 'p':
		typeModelViewMatrix = h_modelViewMatrix;
		typeProjectionMatrix = h_projectionMatrix;
		typePipelineProgram = pipelineProgram;
		typevao = vao;
		typeNumVertices = numVertices;
		drawType = GL_POINTS;
		break;

	case 'l':
		typeModelViewMatrix = lines_h_modelViewMatrix;
		typeProjectionMatrix = lines_h_projectionMatrix;
		typePipelineProgram = linesPipelineProgram;
		typevao = linesvao;
		typeNumVertices = numLineVertices;
		drawType = GL_LINES;
		break;

	case 't':
		typeModelViewMatrix = triangles_h_modelViewMatrix;
		typeProjectionMatrix = triangles_h_projectionMatrix;
		typePipelineProgram = trianglesPipelineProgram;
		typevao = trianglesvao;
		typeNumVertices = numTriVertices;
		drawType = GL_TRIANGLES;
		break;
	}

	typePipelineProgram->Bind(); // bind the pipeline program, must do once before glUniformMatrix4fv

	// write projection and modelview matrix to shader
	GLboolean isRowMajor = GL_FALSE;

	float m[16]; // column-major
	matrix->GetMatrix(m);
	// upload m to the GPU
	glUniformMatrix4fv(typeModelViewMatrix, 1, isRowMajor, m);

	float p[16]; // column-major
	matrix->SetMatrixMode(OpenGLMatrix::Projection);
	matrix->GetMatrix(p);
	// upload p to the GPU
	glUniformMatrix4fv(typeProjectionMatrix, 1, isRowMajor, p);
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);

	glBindVertexArray(typevao); // bind the VAO
	GLint first = 0;
	GLsizei count = typeNumVertices;
	glDrawArrays(drawType, first, count);
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
  matrix->Perspective(15.0f, aspect, 0.01f, 1000.0f); // Hard coded aspect ratio for milestone
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

	case 'p':
	  // points view
	  renderType = 'p';
	break;
	
	case 'l':
	  // lines (wireframe) view
	  renderType = 'l';
	break;

	case 't':
	  // solid triangles view
	  renderType = 't';
	break;
  }
}

void initVAO()
{
	// Triangles
	// create the vao
	glGenVertexArrays(1, &trianglesvao);
	glBindVertexArray(trianglesvao); // bind the VAO

	// bind the VBO “buffer” (must be previously created)
	glBindBuffer(GL_ARRAY_BUFFER, trianglesBuffer);
	// get location index of the “position” shader variable
	GLuint loc = glGetAttribLocation(trianglesProgram, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(trianglesProgram, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
	offset = (const void*) triPositionSize;
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO

	// Lines
	// create the vao
	glGenVertexArrays(1, &linesvao);
	glBindVertexArray(linesvao); // bind the VAO

							// bind the VBO “buffer” (must be previously created)
	glBindBuffer(GL_ARRAY_BUFFER, linesBuffer);
	// get location index of the “position” shader variable
	loc = glGetAttribLocation(linesProgram, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	offset = (const void*)0;
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(linesProgram, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
	offset = (const void*)linePositionSize;
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO

	// Points
	// create the vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // bind the VAO

							// bind the VBO “buffer” (must be previously created)
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	// get location index of the “position” shader variable
	loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	offset = (const void*)0;
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
	offset = (const void*)positionSize;
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO
}

void initPipelineProgram()
{
	// Points
	pipelineProgram = new BasicPipelineProgram();
	pipelineProgram->Init("../openGLHelper-starterCode");
	pipelineProgram->Bind();

	// get a handle to the program
	program = pipelineProgram->GetProgramHandle();

	// initialize uniform variable handles
	// get a handle to the modelViewMatrix shader variable
	h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");

	// do the same for the projectionMatrix
	h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");

	// Lines
	linesPipelineProgram = new BasicPipelineProgram();
	linesPipelineProgram->Init("../openGLHelper-starterCode");
	linesPipelineProgram->Bind();

							 // get a handle to the program
	linesProgram = linesPipelineProgram->GetProgramHandle();

	// initialize uniform variable handles
	// get a handle to the modelViewMatrix shader variable
	lines_h_modelViewMatrix = glGetUniformLocation(linesProgram, "modelViewMatrix");

	// do the same for the projectionMatrix
	lines_h_projectionMatrix = glGetUniformLocation(linesProgram, "projectionMatrix");

	// Triangles
	trianglesPipelineProgram = new BasicPipelineProgram();
	trianglesPipelineProgram->Init("../openGLHelper-starterCode");
	trianglesPipelineProgram->Bind();

							 // get a handle to the program
	trianglesProgram = trianglesPipelineProgram->GetProgramHandle();

	// initialize uniform variable handles
	// get a handle to the modelViewMatrix shader variable
	triangles_h_modelViewMatrix = glGetUniformLocation(trianglesProgram, "modelViewMatrix");

	// do the same for the projectionMatrix
	triangles_h_projectionMatrix = glGetUniformLocation(trianglesProgram, "projectionMatrix");

	initVAO();
}

void initVBO()
{
	// Init Triangles VBO
	glGenBuffers(1, &trianglesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, trianglesBuffer);
	glBufferData(GL_ARRAY_BUFFER, triPositionSize + triColorSize,
		NULL, GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data to it

							   // upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		triPositionSize, triPositions);

	// upload color data
	glBufferSubData(GL_ARRAY_BUFFER, triPositionSize, triColorSize, triColors);

	// Init Lines VBO
	glGenBuffers(1, &linesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, linesBuffer);
	glBufferData(GL_ARRAY_BUFFER, linePositionSize + lineColorSize,
		NULL, GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data to it

							   // upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		linePositionSize, linePositions);

	// upload color data
	glBufferSubData(GL_ARRAY_BUFFER, linePositionSize, lineColorSize, lineColors);
	
	// Init Points VBO
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, positionSize + colorSize,
		NULL, GL_STATIC_DRAW); // init buffer’s size, but don’t assign any data to it

	// upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		positionSize, positions);

	// upload color data
	glBufferSubData(GL_ARRAY_BUFFER, positionSize, colorSize, colors);
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

  int height = heightmapImage->getHeight();
  int width = heightmapImage->getWidth();
  numVertices = height * width;
  numLineVertices = (4 * (height - 1) * (width - 1)) + (2 * (height + width - 2)); // Four vertices = two lines for each vertex, then fill in the edges
  numTriVertices = (6 * (height - 1) * (width - 1)); // 6 vertices = two triangles for each quad of four pixels
  
  positions = new float[numVertices * 3];
  linePositions = new float[numLineVertices * 3];
  triPositions = new float[numTriVertices * 3];

  colors = new float[numVertices * 4];
  lineColors = new float[numLineVertices * 4];
  triColors = new float[numTriVertices * 4];

  positionSize = sizeof(float) * numVertices * 3;
  linePositionSize = sizeof(float) * numLineVertices * 3;
  triPositionSize = sizeof(float) * numTriVertices * 3;

  colorSize = sizeof(float) * numVertices * 4;
  lineColorSize = sizeof(float) * numLineVertices * 4;
  triColorSize = sizeof(float) * numTriVertices * 4;

  float scale = 0.2f;
  int posInd = 0;
  int colorInd = 0;
  int linePosInd = 0;
  int lineColorInd = 0;
  int triPosInd = 0;
  int triColorInd = 0;
  for (int i = 0; i < height; ++i) {
	  for (int j = 0; j < width; ++j) {
		  // Points
		  positions[posInd] = (float)i; // Row
		  positions[posInd + 1] = scale * heightmapImage->getPixel(i, j, 0); // Height
		  positions[posInd + 2] = (float)-j; // Column
		  posInd += 3;

		  colors[colorInd] = 0; // Red
		  colors[colorInd + 1] = heightmapImage->getPixel(i, j, 0) / 255.0f; // Green
		  colors[colorInd + 2] = 0; // Blue
		  colors[colorInd + 3] = 1; // Alpha Channel
		  colorInd += 4;

		  // Lines
		  if (j + 1 < width) {
			  // Draw horizontal line
			  // Positions
			  linePositions[linePosInd] = (float)i; // Row
			  linePositions[linePosInd + 1] = scale * heightmapImage->getPixel(i, j, 0); // Height
			  linePositions[linePosInd + 2] = (float)-j; // Column
			  linePosInd += 3;

			  linePositions[linePosInd] = (float)i; // Row
			  linePositions[linePosInd + 1] = scale * heightmapImage->getPixel(i, j + 1, 0); // Height
			  linePositions[linePosInd + 2] = (float)-(j + 1); // Column
			  linePosInd += 3;

			  // Colors
			  lineColors[lineColorInd] = 0; // Red
			  lineColors[lineColorInd + 1] = heightmapImage->getPixel(i, j, 0) / 255.0f; // Green
			  lineColors[lineColorInd + 2] = 0; // Blue
			  lineColors[lineColorInd + 3] = 1; // Alpha Channel
			  lineColorInd += 4;

			  lineColors[lineColorInd] = 0; // Red
			  lineColors[lineColorInd + 1] = heightmapImage->getPixel(i, j + 1, 0) / 255.0f; // Green
			  lineColors[lineColorInd + 2] = 0; // Blue
			  lineColors[lineColorInd + 3] = 1; // Alpha Channel
			  lineColorInd += 4;
		  }
		  if (i + 1 < height) {
			  // Draw vertical line
			  // Positions
			  linePositions[linePosInd] = (float)i; // Row
			  linePositions[linePosInd + 1] = scale * heightmapImage->getPixel(i, j, 0); // Height
			  linePositions[linePosInd + 2] = (float)-j; // Column
			  linePosInd += 3;

			  linePositions[linePosInd] = (float)i + 1; // Row
			  linePositions[linePosInd + 1] = scale * heightmapImage->getPixel(i + 1, j, 0); // Height
			  linePositions[linePosInd + 2] = (float)-j; // Column
			  linePosInd += 3;

			  // Colors
			  lineColors[lineColorInd] = 0; // Red
			  lineColors[lineColorInd + 1] = heightmapImage->getPixel(i, j, 0) / 255.0f; // Green
			  lineColors[lineColorInd + 2] = 0; // Blue
			  lineColors[lineColorInd + 3] = 1; // Alpha Channel
			  lineColorInd += 4;

			  lineColors[lineColorInd] = 0; // Red
			  lineColors[lineColorInd + 1] = heightmapImage->getPixel(i + 1, j, 0) / 255.0f; // Green
			  lineColors[lineColorInd + 2] = 0; // Blue
			  lineColors[lineColorInd + 3] = 1; // Alpha Channel
			  lineColorInd += 4;
		  }

		  // Triangles
		  if (i + 1 < height && j + 1 < width) {
			  // Going counter-clockwise is important I read somewhere?
			  // Top-left triangle positions
			  triPositions[triPosInd] = (float)i; // Row
			  triPositions[triPosInd + 1] = scale * heightmapImage->getPixel(i, j, 0); // Height
			  triPositions[triPosInd + 2] = (float)-j; // Column
			  triPosInd += 3;

			  triPositions[triPosInd] = (float)i + 1; // Row
			  triPositions[triPosInd + 1] = scale * heightmapImage->getPixel(i + 1, j, 0); // Height
			  triPositions[triPosInd + 2] = (float)-j; // Column
			  triPosInd += 3;

			  triPositions[triPosInd] = (float)i; // Row
			  triPositions[triPosInd + 1] = scale * heightmapImage->getPixel(i, j + 1, 0); // Height
			  triPositions[triPosInd + 2] = (float)-(j + 1); // Column
			  triPosInd += 3;

			  // Bottom-right triangle positions
			  triPositions[triPosInd] = (float)i + 1; // Row
			  triPositions[triPosInd + 1] = scale * heightmapImage->getPixel(i + 1, j + 1, 0); // Height
			  triPositions[triPosInd + 2] = (float)-(j + 1); // Column
			  triPosInd += 3;

			  triPositions[triPosInd] = (float)i; // Row
			  triPositions[triPosInd + 1] = scale * heightmapImage->getPixel(i, j + 1, 0); // Height
			  triPositions[triPosInd + 2] = (float)-(j + 1); // Column
			  triPosInd += 3;

			  triPositions[triPosInd] = (float)i + 1; // Row
			  triPositions[triPosInd + 1] = scale * heightmapImage->getPixel(i + 1, j, 0); // Height
			  triPositions[triPosInd + 2] = (float)-j; // Column
			  triPosInd += 3;

			  //If have constant channels forever
			  //for (int offi = 0; offi < 24; offi += 4) {
				 // triColors[triColorInd + offi] = 0; // Red
				 // triColors[triColorInd + offi + 2] = 0; // Blue
				 // triColors[triColorInd + offi + 3] = 1; // Alpha Channel
			  //}

			  // Top-left triangle colors
			  triColors[triColorInd] = 0; // Red
			  triColors[triColorInd + 1] = heightmapImage->getPixel(i, j, 0) / 255.0f; // Green
			  triColors[triColorInd + 2] = 0; // Blue
			  triColors[triColorInd + 3] = 1; // Alpha Channel
			  triColorInd += 4;

			  triColors[triColorInd] = 0; // Red
			  triColors[triColorInd + 1] = heightmapImage->getPixel(i + 1, j, 0) / 255.0f; // Green
			  triColors[triColorInd + 2] = 0; // Blue
			  triColors[triColorInd + 3] = 1; // Alpha Channel
			  triColorInd += 4;

			  triColors[triColorInd] = 0; // Red
			  triColors[triColorInd + 1] = heightmapImage->getPixel(i, j + 1, 0) / 255.0f; // Green
			  triColors[triColorInd + 2] = 0; // Blue
			  triColors[triColorInd + 3] = 1; // Alpha Channel
			  triColorInd += 4;

			  // Bottom-right triangle colors
			  triColors[triColorInd] = 0; // Red
			  triColors[triColorInd + 1] = heightmapImage->getPixel(i + 1, j + 1, 0) / 255.0f; // Green
			  triColors[triColorInd + 2] = 0; // Blue
			  triColors[triColorInd + 3] = 1; // Alpha Channel
			  triColorInd += 4;

			  triColors[triColorInd] = 0; // Red
			  triColors[triColorInd + 1] = heightmapImage->getPixel(i, j + 1, 0) / 255.0f; // Green
			  triColors[triColorInd + 2] = 0; // Blue
			  triColors[triColorInd + 3] = 1; // Alpha Channel
			  triColorInd += 4;

			  triColors[triColorInd] = 0; // Red
			  triColors[triColorInd + 1] = heightmapImage->getPixel(i + 1, j, 0) / 255.0f; // Green
			  triColors[triColorInd + 2] = 0; // Blue
			  triColors[triColorInd + 3] = 1; // Alpha Channel
			  triColorInd += 4;
		  }
	  }
  }

  renderType = 'p';
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


