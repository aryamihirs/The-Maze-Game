#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SFML/Audio.hpp>
using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			1,                  // attribute 1. Color
			3,                  // size (r,g,b)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/
float l = 1.0f;
float b = 1.0f;
float h = 1.0f;
float lf = 1.0f;
float bf = 1.0f;
float hf = 1.0f;
float lo = 1.0f;
float bo = 1.0f;
float ho = 1.0f;
float lw = 1.0f;
float bw = 1.0f;
float hw = 1.0f;
float x_cuboid = 0;
float y_cuboid = 0;
float z_cuboid = 19.0f;
float x_offset = 0;
float y_offset = 3.0f;
float z_offset = 3.0f;
float x_target;
float y_target;
float z_target;
float x_cam = x_cuboid + x_offset;
float y_cam = y_cuboid + y_offset;
float z_cam = z_cuboid + z_offset;
float x_axis = 0;
float y_axis = 1;
float z_axis = 0;
int follow_flag = 1;
int up_fl = 0;
int down_fl = 0;
int right_fl = 0;
int left_fl = 0;
int adv_fl = 0;
int tower_fl = 0;
int top_fl = 0;
int heli_fl = 0;
int jump_fl = 0;
int dir_up = 1;
void enableTopcam()
{
	x_cam = 10;
	y_cam = 15;
	z_cam = 10;
	x_axis = 0;
	y_axis = 0;
	z_axis = -1;
	x_target = x_cuboid;
	y_target = y_cuboid;
	z_target = z_cuboid;

	follow_flag = 0;
	adv_fl = 0;
	tower_fl = 0;
	heli_fl = 0;
}
void enableTowercam()
{
	x_cam = 0;
	y_cam = 5.0f;
	z_cam = 20.0f;
	x_axis = 0;
	y_axis = 1;
	z_axis = 0;
	x_target = x_cuboid;
	y_target = y_cuboid;
	z_target = z_cuboid;
	follow_flag = 0;
	adv_fl = 0;
	top_fl = 0;
	heli_fl = 0;
}
void enableFollowcam()
{
	x_cam = x_cuboid + x_offset;
	y_cam = y_cuboid + y_offset; 
	z_cam = z_cuboid + z_offset; 
	x_axis = 0;
	y_axis = 1;
	z_axis = 0;
	x_target = x_cuboid;
	y_target = y_cuboid;
	z_target = z_cuboid;
	adv_fl = 0;
	tower_fl = 0;
	heli_fl = 0;
}

void enableAdvcam()
{
	x_cam = x_cuboid + 0.5f;
	y_cam = y_cuboid + 1.5f;
	z_cam = z_cuboid + 0.5f;
	x_axis = 0;
	y_axis = 1;
	z_axis = 0;
	if(right_fl == 1)
	{
		x_target = 20.0f;
		y_target = y_cuboid;
		z_target = z_target;
	}
	if(left_fl == 1)
	{
		x_target = 0.0f;
		y_target = y_cuboid;
		z_target = z_target;
	}
	if(up_fl == 1)
	{
		x_target = x_cuboid;
		y_target = y_cuboid;
		z_target = 0;
	}
	if(down_fl == 1)
	{
		x_target = x_cuboid;
		y_target = y_cuboid;
		z_target = 20.0f;
	}
	follow_flag = 0;
	top_fl = 0;
	tower_fl = 0;
	heli_fl = 0;
}

bool checkIfFloor(float floor_y)
{
	if(floor_y == y_cuboid && x_cuboid > 0 && x_cuboid < 17 && z_cuboid > 0 && z_cuboid < 20)
		return true;
	else 
		return false;
}

bool checkIfMovableFloor(float x_mov_floor, float z_mov_floor)
{
	if(x_cuboid < x_mov_floor + 1.9f && x_cuboid > x_mov_floor - 0.9f && z_cuboid < z_mov_floor + 1.9f && z_cuboid > z_mov_floor - 0.9f)
		return true;
	else
		return false;
}
bool checkIfObs(float x_obs, float z_obs)
{
	/*if(down_fl == 1 && z_cuboid + 1 > z_obs && x_cuboid < x_obs + 1.9f && x_cuboid > x_obs - 0.9f)
		return true;
	else if(up_fl == 1 && z_cuboid < z_obs + 1 && x_cuboid < x_obs + 1.9f && x_cuboid > x_obs - 0.9f)
		return true;
    else if(right_fl == 1 && x_cuboid + 1 > x_obs && z_cuboid < z_obs + 1.9f && z_cuboid > z_obs - 0.9f)
		return true;
	else if(left_fl == 1 && x_cuboid < x_obs + 1 && z_cuboid < z_obs + 1.9f && z_cuboid > z_obs - 0.9f)
		return true;*/
	if(x_cuboid > x_obs && x_cuboid < x_obs + 1 && z_cuboid > z_obs && z_cuboid < z_obs + 1)
		return true;
	else
		return false;
}
/*void enableHelicoptercam()  {

  }
 */
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_UP:
				z_cuboid -= 0.2f;
				up_fl = 1;
				down_fl = 0;
				right_fl = 0;
				left_fl = 0;
				break;
			case GLFW_KEY_DOWN:
				z_cuboid += 0.2f;
				up_fl = 0;
				down_fl = 1;
				right_fl = 0;
				left_fl = 0;
				break;
			case GLFW_KEY_RIGHT:
				x_cuboid += 0.2f;
				up_fl = 0;
				down_fl = 0;
				right_fl = 1;
				left_fl = 0;
				break;
			case GLFW_KEY_LEFT:
				up_fl = 0;
				down_fl = 0;
				right_fl = 0;
				left_fl = 1;
				x_cuboid -= 0.2f;
				break;
			case GLFW_KEY_SPACE:
				jump_fl = 1;
				break;
			case GLFW_KEY_T:
				tower_fl = 1;
				top_fl = 0;
				adv_fl = 0;
				follow_flag = 0;
				heli_fl = 0;
				break;
			case GLFW_KEY_P:
				tower_fl = 0;

				adv_fl = 0;
				follow_flag = 0;
				heli_fl = 0;
				top_fl = 1;
				break;

			case GLFW_KEY_A:
				adv_fl = 1;
				tower_fl = 0;
				top_fl = 0;
				follow_flag = 0;
				heli_fl = 0;
				break;
				//case GLFW_KEY_H:
				//	enableHelicoptercam();
				//	break;
			case GLFW_KEY_F:
				follow_flag = 1;
				adv_fl = 0;
				tower_fl = 0;
				top_fl = 0;
				heli_fl = 0;

				break;
				//   case GLFW_KEY_SPACE:

				//       break;
			default:
				break;
		}
	}

	else if (action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_UP:
				z_cuboid -= 0.2f;
				break;
			case GLFW_KEY_DOWN:
				z_cuboid += 0.2f;
				break;
			case GLFW_KEY_RIGHT:
				x_cuboid += 0.2f;
				break;
			case GLFW_KEY_LEFT:
				x_cuboid -= 0.2f;
				break;
				//   case GLFW_KEY_SPACE:

				//       break;
			default:
				break;
		}
	}

	else if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;
			default:
				break;
		}
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			quit(window);
			break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			if (action == GLFW_RELEASE)
				//	triangle_rot_dir *= -1;
				break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_RELEASE) {
				//	rectangle_rot_dir *= -1;
			}
			break;
		default:
			break;
	}
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	/* With Retina display on Mac OS X, GLFW's FramebufferSize
	   is different from WindowSize */
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

	// Ortho projection for 2D views
	//Matrices.projection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 500.0f);
}
/*class Cuboid
  {


  void createCuboid(float l, float b, float h)
  {
  static const GLfloat vertex_buffer_data [] = {
  0,0,0,
  l,0,0,
  l,b,0,

  l,b,0,
  0,b,0,
  0,0,0,

  0,0,0,
  0,b,0,
  0,b,h,

  0,b,h,
  0,0,h,
  0,0,0,

  0,0,h,
  0,b,h,
  l,b,h,

  l,b,h,
  l,0,h,
  0,0,h,

  l,0,h,
  l,b,h,
  l,b,0,

  l,b,0,
  l,0,0,
  l,0,h,

  0,b,0,
  0,b,h,
  l,b,h

  l,b,h,
  l,b,0,
  0,b,0,

  0,0,0,
  0,0,h,
  l,0,h,

  l,0,h,
  l,0,0,
  0,0,0
  };
  }
  };
 */
VAO *cuboid, *zameen, *obs, *paani;

void createCuboid()
{
	static const GLfloat vertex_buffer_data [] = {
		0,0,0,
		l,0,0,
		l,b,0,

		l,b,0,
		0,b,0,
		0,0,0,

		0,0,0,
		0,b,0,
		0,b,h,

		0,b,h,
		0,0,h,
		0,0,0,

		0,0,h,
		0,b,h,
		l,b,h,

		l,b,h,
		l,0,h,
		0,0,h,

		l,0,h,
		l,b,h,
		l,b,0,

		l,b,0,
		l,0,0,
		l,0,h,

		0,b,0,
		0,b,h,
		l,b,h,

		l,b,h,
		l,b,0,
		0,b,0,

		0,0,0,
		0,0,h,
		l,0,h,

		l,0,h,
		l,0,0,
		0,0,0
	};
	static const GLfloat color_buffer_data[] = { 
		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,

		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,

		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,
		1,0.8f,1,

		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,
		1,0.8f,0.6f,

		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1,

		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1,
		0.6,0.75,1

	};
	cuboid = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createFloor()
{
	static const GLfloat vertex_buffer_data [] = {
		0,0,0,
		lf,0,0,
		lf,bf,0,

		lf,bf,0,
		0,bf,0,
		0,0,0,

		0,0,0,
		0,bf,0,
		0,bf,hf,

		0,bf,hf,
		0,0,hf,
		0,0,0,

		0,0,hf,
		0,bf,hf,
		lf,bf,hf,

		lf,bf,hf,
		lf,0,hf,
		0,0,hf,

		lf,0,hf,
		lf,bf,hf,
		lf,bf,0,

		lf,bf,0,
		lf,0,0,
		lf,0,hf,

		0,bf,0,
		0,bf,hf,
		lf,bf,hf,

		lf,bf,hf,
		lf,bf,0,
		0,bf,0,

		0,0,0,
		0,0,hf,
		lf,0,hf,

		lf,0,hf,
		lf,0,0,
		0,0,0
	};
	static const GLfloat color_buffer_data[] = { 
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,

		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,

		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,

		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,

		0,  0.5f,0,
		0.5f,  0.5f, 0,
		0, 0.5f, 0,
		0,  0.5f, 0,
		0.5f,  0.5f, 0,
		0,  0.5f, 0,  

		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0,
		0.6f,0.2f,0
	};
	zameen = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createWater()
{
	static const GLfloat vertex_buffer_data [] = {
		0,0,0,
		lw,0,0,
		lw,bw,0,

		lw,bw,0,
		0,bw,0,
		0,0,0,

		0,0,0,
		0,bw,0,
		0,bw,hw,

		0,bw,hw,
		0,0,hw,
		0,0,0,

		0,0,hw,
		0,bw,hw,
		lw,bw,hw,

		lw,bw,hw,
		lw,0,hw,
		0,0,hw,

		lw,0,hw,
		lw,bw,hw,
		lw,bw,0,

		lw,bw,0,
		lw,0,0,
		lw,0,hw,

		0,bw,0,
		0,bw,hw,
		lw,bw,hw,

		lw,bw,hw,
		lw,bw,0,
		0,bw,0,

		0,0,0,
		0,0,hw,
		lw,0,hw,

		lw,0,hw,
		lw,0,0,
		0,0,0
	};
	static const GLfloat color_buffer_data[] = { 
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,

		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,

		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		
		0.3f,  0.58f,1.0f,
		0.0f,  0.23f, 0.6f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.0f,  0.23f, 0.6f,
		0.3f,  0.58f,1.0f,

		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		0.3f,  0.58f,1.0f,
		
	};
	paani = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createObs()
{
	static const GLfloat vertex_buffer_data [] = {
		0,0,0,
		lo,0,0,
		lo,bo,0,

		lo,bo,0,
		0,bo,0,
		0,0,0,

		0,0,0,
		0,bo,0,
		0,bo,ho,

		0,bo,ho,
		0,0,ho,
		0,0,0,

		0,0,ho,
		0,bo,ho,
		lo,bo,ho,

		lo,bo,ho,
		lo,0,ho,
		0,0,ho,

		lo,0,ho,
		lo,bo,ho,
		lo,bo,0,

		lo,bo,0,
		lo,0,0,
		lo,0,ho,

		0,bo,0,
		0,bo,ho,
		lo,bo,ho,

		lo,bo,ho,
		lo,bo,0,
		0,bo,0,

		0,0,0,
		0,0,ho,
		lo,0,ho,

		lo,0,ho,
		lo,0,0,
		0,0,0
	};
	static const GLfloat color_buffer_data[] = { 
		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,

		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,

		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,
		0.2f,0.06f,0,

		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,
		0.3f,0.09f,0,

		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0,

		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0,
		0.4f,  0.12f,0

	};
	obs = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void drawCuboid()
{
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye (x_cam,y_cam,z_cam);
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (x_target, y_target, z_target);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (x_axis, y_axis, z_axis);

	// Compute Camera matrix (view)
	Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	//Matrices.view = glm::lookAt(glm::vec3(10,10,10), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	// Load identity to model matrix
	Matrices.model = glm::mat4(1.0f);

	/* Render your scene */

	glm::mat4 translateCuboid = glm::translate (glm::vec3(x_cuboid, y_cuboid, z_cuboid )); // glTranslatef
	//glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 cuboidTransform = translateCuboid;
	Matrices.model *= cuboidTransform; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(cuboid);

}

void drawFloor(int x_floor, int y_floor, int z_floor)
{

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye (x_cam,y_cam,z_cam);
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (x_target, y_target, z_target);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (x_axis, y_axis, z_axis);

	// Compute Camera matrix (view)
	Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	//Matrices.view = glm::lookAt(glm::vec3(10,10,10), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model


	Matrices.model = glm::mat4(1.0f);

	/* Render your scene */

	glm::mat4 translateFloor = glm::translate (glm::vec3(x_floor,y_floor,z_floor)); // glTranslatef
	//glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 floorTransform = translateFloor;
	Matrices.model *= floorTransform; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(zameen);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
}
void drawPaani(int x_paani, int y_paani, int z_paani)
{

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye (x_cam,y_cam,z_cam);
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (x_target, y_target, z_target);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (x_axis, y_axis, z_axis);

	// Compute Camera matrix (view)
	Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	//Matrices.view = glm::lookAt(glm::vec3(10,10,10), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model


	Matrices.model = glm::mat4(1.0f);

	/* Render your scene */

	glm::mat4 translatePaani = glm::translate (glm::vec3(x_paani,y_paani,z_paani)); // glTranslatef
	//glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 paaniTransform = translatePaani;
	Matrices.model *= paaniTransform; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(paani);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
}
void drawObs(int x_obs, int z_obs)
{

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye (x_cam,y_cam,z_cam);
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (x_target, y_target, z_target);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (x_axis, y_axis, z_axis);

	// Compute Camera matrix (view)
	Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	//Matrices.view = glm::lookAt(glm::vec3(10,10,10), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model


	Matrices.model = glm::mat4(1.0f);

	/* Render your scene */

	glm::mat4 translateObs = glm::translate (glm::vec3(x_obs,0,z_obs)); // glTranslatef
	//glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 obsTransform = translateObs;
	Matrices.model *= obsTransform; 
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(obs);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
}


/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	   is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	/* Objects should be created before any other gl function and shaders */
	// Create the models
	createCuboid();
	createFloor();
	createObs();
	createWater();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (1, 1,1, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

//sf::SoundBuffer buffer;
//	sf::Sound sound;

int main (int argc, char** argv)
{
	//bg music
	
	sf::Music music;
	if (!music.openFromFile("bg.ogg"))
		return -1; // error
	music.play();
	
	float t = 0 ;
	int width = 600;
	int height = 600;
	int i,k,x[20],z[20],obsx[20],obsz[20],o;
	float farsh_m_y = -4.0f;
	float farsh_y = -1.0f;
	int farsh_up_fl = 1;
	GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

	double last_update_time = glfwGetTime(), current_time;
	for(i=0; i<20; i++)
	{
		obsx[i] = (rand() % 20);
		x[i] = (rand() % 20);
		z[i] = (rand() % 20);
		obsz[i] = (rand() % 20);
	}

	/* Draw in loop */
	while (!glfwWindowShouldClose(window)) {
		if(farsh_m_y < 2.0f && farsh_up_fl == 1)
			farsh_m_y += 0.005f;

		if(farsh_m_y >= 2.0f && farsh_up_fl == 1)
			farsh_up_fl = 0;

		if(farsh_m_y > -4.0f && farsh_up_fl == 0)
			farsh_m_y	 -= 0.005f;
			
		if(farsh_m_y <= -4.0f && farsh_up_fl == 0)
			farsh_up_fl = 1;

		// OpenGL Draw commands
		if(follow_flag == 1)
			enableFollowcam();
		else
			follow_flag = 0;

		if(adv_fl == 1)
			enableAdvcam();
		else
			adv_fl = 0;
		if(top_fl == 1)
			enableTopcam();
		else
			top_fl = 0;
		if(tower_fl == 1)
			enableTowercam();
		else
			tower_fl = 0;
/*
		for(i=0; i < 17; i++)
			for(k=0; k < 20; k++)
			{
				if(checkIfFloor(farsh_y + 1.0f))
				{
					cout << "ping\n";
					if(jump_fl == 1 && y_cuboid < farsh_y + 2.0f)
					{
						cout << "ping1\n";
						if(dir_up == 1)
	 						y_cuboid += 0.1f;
	 					else
	 					{
	 						y_cuboid -= 0.1f;
	 						if(y_cuboid < farsh_y + 1)
	 							jump_fl = 0;
	 					}
	 				}
	 				else if(jump_fl == 1 && y_cuboid >= farsh_y + 2.0f)
	 					dir_up = 0;
	 			}


 				if(checkIfMovableFloor((float)x[i],(float)z[k]))
 				{
					cout << "ping\n";

 					y_cuboid = farsh_m_y;
 					if(jump_fl == 1 && y_cuboid < farsh_m_y + 2.0f)
 						if(dir_up == 1)
	 						y_cuboid += 0.1f;
	 					else
	 					{
	 						y_cuboid -= 0.1f;
	 						if(y_cuboid < farsh_m_y + 1)
	 							jump_fl = 0;
	 					}
	 				else if(jump_fl == 1 && y_cuboid >= farsh_m_y + 2.0f)
	 					dir_up = 0;
				}
				if(checkIfObs((float)x[i],(float)z[k]))
				{
					x_cuboid = 0;
					z_cuboid = 19.0f;
				}
*/	
		if(jump_fl == 1 || (in_air == 0 && on_gr == 1))


		if(x_cuboid < -1.0f || x_cuboid > 18.0f || z_cuboid < -1.0f || z_cuboid > 20.0f)
			{
				x_cuboid = 0;
				z_cuboid = 19;
			}

		drawCuboid();	

		for(i=0; i < 17; i++)
			for(k=0; k < 20; k++)
			{
				if(i != x[i] && k != z[i])
					drawFloor(i,farsh_y,k);
				else
					drawFloor(i,farsh_m_y,k);

				if(checkIfObs((float)x[i],(float)z[k]))
				{
					x_cuboid = 0;
					z_cuboid = 19.0f;
				}
			}

		for(i=-20;i<40;i++)
			for(k=-20; k<40; k++)
				drawPaani(i,-3.0f,k);

		

		for(o=0; o<17; o++)
		{
			drawObs(obsx[o], obsz[o]);
		}
		

		// Swap Frame Buffer in double buffering
		glfwSwapBuffers(window);

		// Poll for Keyboard and mouse events
		glfwPollEvents();

		// Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
		current_time = glfwGetTime(); // Time in seconds
		if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
			// do something every 0.5 seconds ..
			last_update_time = current_time;
		}
	}
	glfwTerminate();
	exit(EXIT_SUCCESS);

}
