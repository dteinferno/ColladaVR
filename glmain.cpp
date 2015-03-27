////////////////////////////////////////////////////////////////////////////////////
//                 Main Routine for Drawing Objects via OpenGL                    //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/15/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

#include "glmain.h"
#include <windows.h>
#include "winmain.h"
#include "system.h"
#include "balloffset.h"

// Define constants related to the shapes
float dist2stripe = 10;
float fovAng = 80 * M_PI / 180;
float numAng = 100.0f;

// Define offset values (Rotational, Forward, and Lateral)
float BallOffsetRotNow = 0.0f;
float BallOffsetForNow = 0.0f;
float BallOffsetSideNow = 0.0f;

// Variable to control the direction of the open loop stripe (+/-1) AND whether or not to display anything at all (0)
int olsdir;

// Vertex shader which passes through the texture and position coordinates
const char* vertex_shader =
"#version 400\n"
"in vec4 position;"
"in vec2 texcoord;"
"uniform mat4 Projection;"
"uniform mat4 View;"
"uniform mat4 Model;"
"out vec2 Texcoord;"
"void main () {"
"  Texcoord = texcoord;"
"  gl_Position = Projection * View * Model * position;"
"}";

// Fragment shader which calculates the desired distortion (position and brightness) for projecting onto a cylindrical screen
const char* fragment_shader =
"#version 400\n"
"in vec2 Texcoord;"
"out vec4 frag_colour;"
"uniform int color;"
"uniform float cyl;"
"uniform int projnum;"
"uniform sampler2D tex;"
"void main () {"

"  float LtoScreen = 6.55;"
"  float wofScreen = 1.93;"
"  float proj0power = 1.14;"
"  float proj1power = 1.07;"
"  float proj2power = 1.04;"
"  float projnorm = min(proj0power, min(proj1power, proj2power));"

"  float angofScreen = 4*3.141592/9;"

"  float texshifts = wofScreen*(3*Texcoord.s-1.5);"
" if (Texcoord.s < 0.3333)"
"  texshifts = wofScreen*(3*Texcoord.s-0.5);"
" if (Texcoord.s > 0.6666)"
"  texshifts = wofScreen*(3*Texcoord.s-2.5);"

"  float texshiftt = Texcoord.t-0.5;"
"  float modtfactor = (1.5 - sqrt(1.5*1.5-texshifts*texshifts))/LtoScreen + 1.0;"
"  float distorttinit = texshiftt*modtfactor + 0.5;"

"  float texchop = 3*Texcoord.s;"
" if (Texcoord.s > 0.3333)"
"  texchop = texchop - 1;"
" if (Texcoord.s > 0.6666)"
"  texchop = texchop - 1;"

"  float distortsinit = 0.3881*pow(texchop,4)-0.1844*pow(texchop,3) - 0.2624*pow(texchop,2)+1.049*texchop;"

"  float distorts = 0.5*(1+ tan(angofScreen*(distortsinit - 0.5))/tan(0.5*angofScreen));"
"  float distortt = 0.5 + (distorttinit - 0.5) * cos(0.5 * angofScreen)/cos(angofScreen*(distorts-0.5));"

"  distorts = 0.333 * distorts; "
" if (Texcoord.s > 0.3333)"
"  distorts = distorts + 0.333;"
" if (Texcoord.s > 0.6666)"
"  distorts = distorts + 0.333;"

"  distortt = distorttinit;"

"  float brightcorrect = 1.243*pow(distorts,4)-1.328*pow(distorts,3) + 0.8553*pow(distorts,2)-0.3047*distorts+0.5221;"

"  if (cyl == 0.0)"
"  {"
"   distorts = Texcoord.s;"
"   distortt = Texcoord.t;"
"   brightcorrect = 1;"
"  }"

" float projcorrect = projnorm/proj1power;"
" if (Texcoord.s < 0.3333)"
"  projcorrect = projnorm/proj2power;"
" if (Texcoord.s > 0.6666)"
"  projcorrect = projnorm/proj0power;"

" if (projnum == 100)"
" {"
"  projcorrect = 1.0;"
" }"

"  vec2 distort = vec2 (distorts, distortt);"
"  vec4 unmodColor = texture(tex, distort);"
" if (color == 1)"
" {"
"  unmodColor.g = 0.0;"
"  unmodColor.b = 0.0;"
" }"
" if (color == 2)"
" {"
"  unmodColor.r = 0.0;"
"  unmodColor.b = 0.0;"
" }"
" if (color == 3)"
" {"
"  unmodColor.r = 0.0;"
"  unmodColor.g = 0.0;"
" }"
"  frag_colour = projcorrect*brightcorrect*vec4(unmodColor.r*1.0, unmodColor.g*1.0, unmodColor.b*1.0, 1.0 );"
"}";

// Define the OpenGL constants
GLuint vao[4];
GLuint vbo;
GLuint vs;
GLuint fs;
GLuint shader_program;
GLuint *tex;
GLuint cylLocation;
GLuint ProjNumber;
GLuint setColor;
glm::mat4 ProjectionMatrix;
GLuint ProjectionID;
glm::mat4 ViewMatrix;
GLuint ViewID;
glm::mat4 ModelMatrix;
GLuint ModelID;

// Constants for loading objects from the shader
GLuint vbo_obj1;
GLuint uvbuffer_obj1;
GLuint normalsbuffer_obj1;
std::vector<glm::vec3> vertices_obj1;
std::vector<glm::vec2> uvs_obj1;
std::vector<glm::vec3> normals_obj1;

GLuint vbo_obj2;
GLuint uvbuffer_obj2;
GLuint normalsbuffer_obj2;
std::vector<glm::vec3> vertices_obj2;
std::vector<glm::vec2> uvs_obj2;
std::vector<glm::vec3> normals_obj2;

// InitOpenGL: initializes OpenGL; defines buffers, constants, etc...
void InitOpenGL(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);


	// Create the shaders
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	glCompileShader(vs);

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, NULL);
	glCompileShader(fs);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, fs);
	glAttachShader(shader_program, vs);
	glLinkProgram(shader_program);
	glUseProgram(shader_program);

	// Initialize the vertex array object for the object
	glGenVertexArrays(1, &vao[0]);
	glBindVertexArray(vao[0]);

	// Read our first .obj file
	bool resdat = loadOBJ("d://OpenGL//BlenderObjects//Cylinder.obj", vertices_obj1, uvs_obj1, normals_obj1);

	glGenBuffers(1, &vbo_obj1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_obj1);
	glBufferData(GL_ARRAY_BUFFER, vertices_obj1.size() * sizeof(glm::vec3), &vertices_obj1[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer_obj1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_obj1);
	glBufferData(GL_ARRAY_BUFFER, uvs_obj1.size() * sizeof(glm::vec2), &uvs_obj1[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalsbuffer_obj1);
	glBindBuffer(GL_ARRAY_BUFFER, normalsbuffer_obj1);
	glBufferData(GL_ARRAY_BUFFER, normals_obj1.size() * sizeof(glm::vec3), &normals_obj1[0], GL_STATIC_DRAW);

	// Create a pointer for the position
	glBindBuffer(GL_ARRAY_BUFFER, vbo_obj1);
	GLint posAttrib = glGetAttribLocation(shader_program, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Create a pointer for the texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_obj1);
	GLint texAttrib = glGetAttribLocation(shader_program, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Initialize the vertex array object for the object
	glGenVertexArrays(1, &vao[1]);
	glBindVertexArray(vao[1]);

	// Read our first .obj file
	resdat = loadOBJ("d://OpenGL//BlenderObjects//FrontCyl.obj", vertices_obj2, uvs_obj2, normals_obj2);

	// Read our second .obj file
	resdat = loadOBJ("d://OpenGL//BlenderObjects//BackCyl.obj", vertices_obj2, uvs_obj2, normals_obj2);

	glGenBuffers(1, &vbo_obj2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_obj2);
	glBufferData(GL_ARRAY_BUFFER, vertices_obj2.size() * sizeof(glm::vec3), &vertices_obj2[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer_obj2);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_obj2);
	glBufferData(GL_ARRAY_BUFFER, uvs_obj2.size() * sizeof(glm::vec2), &uvs_obj2[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalsbuffer_obj2);
	glBindBuffer(GL_ARRAY_BUFFER, normalsbuffer_obj2);
	glBufferData(GL_ARRAY_BUFFER, normals_obj2.size() * sizeof(glm::vec3), &normals_obj2[0], GL_STATIC_DRAW);

	// Create a pointer for the position
	glBindBuffer(GL_ARRAY_BUFFER, vbo_obj2);
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Create a pointer for the texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_obj2);
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Initialize the vertex array object for the object
	//glGenVertexArrays(1, &vao[2]);
	//glBindVertexArray(vao[2]);

	// Create our distorted screen
	// Initialize the vertex array object
	glGenVertexArrays(1, &vao[3]);
	glBindVertexArray(vao[3]);

	glGenBuffers(1, &vbo);

	// Define constants for defining the shapes
	float windowSpan =  dist2stripe * tanf(fovAng / 2);
	float aspect = float(SCRWIDTH) / float(SCRHEIGHT);

	// Define the vertices
	float vertices[] = {
		// The first four points are the position, while the final two are the texture coordinates
		// First set of points is for the undistorted vertical stripe.
		-2.0f, (float)dist2stripe, -40.0f, 1.0f, 1.0f, 1.0f,
		-2.0f, (float)dist2stripe, 40.0f, 1.0f, 1.0f, 0.0f,
		2.0f, (float)dist2stripe, -40.0f, 1.0f, 0.0f, 1.0f,
		2.0f, (float)dist2stripe, 40.0f, 1.0f, 0.0f, 0.0f,

		// Second set of points for the final display that the texture will be mapped onto.
		-windowSpan, (float)dist2stripe, -windowSpan / aspect, 1.0f, 0.0f, 1.0f,
		-windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 0.0f, 0.0f,
		windowSpan, (float)dist2stripe, -windowSpan / aspect, 1.0f, 1.0f, 1.0f,
		windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 1.0f, 0.0f,

		// Third set of points for a blinking dot that will trigger the photodiode.
		-0.95*windowSpan, (float)dist2stripe, windowSpan * (1 / aspect - 0.05), 1.0f, 1.0f, 1.0f,
		-0.95*windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 1.0f, 0.0f,
		-windowSpan, (float)dist2stripe, windowSpan * (1 / aspect - 0.05), 1.0f, 0.0f, 1.0f,
		-windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 0.0f, 0.0f,

		// Add inner ground floor triangle
		0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		(float) 0.8*dist2stripe*sin(M_PI / numAng), (float) 0.8*dist2stripe*cos(M_PI / numAng), 1.0f, 1.0f, 0.0f, 1.0f,
		-(float) 0.8*dist2stripe*sin(M_PI / numAng), (float) 0.8*dist2stripe*cos(M_PI / numAng), 1.0f, 1.0f, 0.0f, 0.0f,

		// Add outer ground floor polygon
		-(float) 0.95*dist2stripe*sin(M_PI / numAng), (float) 0.95*dist2stripe*cos(M_PI / numAng), 1.0f, 1.0f, 1.0f, 1.0f,
		-(float)dist2stripe*sin(M_PI / numAng), (float)dist2stripe*cos(M_PI / numAng), 1.0f, 1.0f, 1.0f, 0.0f,
		(float) 0.95*dist2stripe*sin(M_PI / numAng), (float) 0.95*dist2stripe*cos(M_PI / numAng), 1.0f, 1.0f, 0.0f, 1.0f,
		(float)dist2stripe*sin(M_PI / numAng), (float)dist2stripe*cos(M_PI / numAng), 1.0f, 1.0f, 0.0f, 0.0f,

		// Add bottom wall points
		-(float) 0.8*dist2stripe*sin(M_PI / numAng), (float) 0.8*dist2stripe*cos(M_PI / numAng), 3.0f, 1.0f, 1.0f, 1.0f,
		-(float) 0.95*dist2stripe*sin(M_PI / numAng), (float) 0.95*dist2stripe*cos(M_PI / numAng), 3.0f, 1.0f, 1.0f, 0.0f,
		(float) 0.8*dist2stripe*sin(M_PI / numAng), (float) 0.8*dist2stripe*cos(M_PI / numAng), 3.0f, 1.0f, 0.0f, 1.0f,
		(float) 0.95*dist2stripe*sin(M_PI / numAng), (float) 0.95*dist2stripe*cos(M_PI / numAng), 3.0f, 1.0f, 0.0f, 0.0f,
	};

	// Bind the vertex data to the buffer array
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Initialize the element array buffer
	GLuint ebo;
	glGenBuffers(1, &ebo);

	// Define the elements for the stripe and the projected screen.
	GLuint elements[] = {
		0, 1, 2,
		1, 2, 3,

		6, 5, 4,
		5, 6, 7,

		8, 9, 10,
		11, 10, 9,

		12, 13, 14,
		15, 16, 17,
		16, 17, 18,

		19, 20, 21,
		20, 21, 22,
		13, 14, 19,
		14, 19, 21,
		15, 17, 20,
		17, 20, 22,
	};

	//Bind the element data to the array.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// Create a pointer for the position
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

	// Create a pointer for the texture coordinates
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));


	// Create a texture array
	tex = new GLuint[5];
	glGenTextures(5, tex);

	// Uniform white texture
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	float pixelsW[] = {
		1.0f, 1.0f, 1.0f,
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, pixelsW);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	// Prep the remaining textures to be used for cylindrical distortion
	for (int n = 0; n < 3; n++) {
		glBindTexture(GL_TEXTURE_2D, tex[1 + n]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// Load a texture using the SOIL loader
	int texWidth, texHeight;
	unsigned char* texImage = SOIL_load_image("d://OpenGL//Textures//SqNoise.png", &texWidth, &texHeight, 0, SOIL_LOAD_RGB);

	glBindTexture(GL_TEXTURE_2D, tex[4]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, texImage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	SOIL_free_image_data(texImage);

	// Pull out the cylindrical distortion switch uniform from the shader
	cylLocation = glGetUniformLocation(shader_program, "cyl");

	// Pull out the projector switch to set the brightness
	ProjNumber = glGetUniformLocation(shader_program, "projnum");

	// Pull out the color setting
	setColor = glGetUniformLocation(shader_program, "color");

	// Pull out the matrices
	ProjectionID = glGetUniformLocation(shader_program, "Projection");
	ViewID = glGetUniformLocation(shader_program, "View");
	ModelID = glGetUniformLocation(shader_program, "Model");


}

void RenderFrame(int direction)
{
	glm::mat4 identity;

	// Map the desired image onto the three different colors
	for (int n = 0; n < 3; n++) {

		//Clear the image and bind the appropriate color texture
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set the background color to black
		glClearDepth(1.0f);
		glBindTexture(GL_TEXTURE_2D, tex[4]); // Bind the appropriate color texture
		glUniform1f(setColor, (int)n+1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers
		glUniform1f(cylLocation, (float) 0.0f); // Initially, we want an undistorted projection
		glUniform1f(ProjNumber, (int)100);  // No brightness correction the first time

		// Take a picture for each of the three camera angles
		for (int windowNum = 0; windowNum < 3; windowNum++) {

			// Define the scene to be captured
			glViewport(SCRWIDTH*windowNum / 3, 0, SCRWIDTH / 3, SCRHEIGHT); // Restrict the viewport to the region of interest
			ProjectionMatrix = glm::perspective(float(360 / M_PI * atanf(tanf(fovAng / 2) * float(SCRHEIGHT) / float(SCRWIDTH))), float(SCRWIDTH) / float(SCRHEIGHT), 0.1f, 1000.0f); //Set the perspective for the projector
			ViewMatrix = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3((windowNum - 1) * tanf(fovAng), 1, 0), glm::vec3(0, 0, 1)); //Look at the appropriate direction for the projector
			glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(ProjectionMatrix));
			glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(ViewMatrix));

			if (closed) // Advance the environment according to the ball movement
			{
				io_mutex.lock();
				BallOffsetRotNow =  BallOffsetRot;
				BallOffsetForNow =  BallOffsetFor;
				BallOffsetSideNow = BallOffsetSide;
				io_mutex.unlock();
			}
			else  // Advance the environment according to open loop parameters
			{
				TimeOffset(BallOffsetRotNow, direction, CounterStart);
				BallOffsetForNow = 0.0f;
				BallOffsetSideNow = 0.0f;
			}

			if (stopped) // Keep the scene fixed
			{
				BallOffsetRotNow = 0.0f;
				BallOffsetForNow = 0.0f;
				BallOffsetSideNow = 0.0f;
			}
			float NormObjVectX = BallOffsetSideNow / sqrt(pow(BallOffsetForNow, 2) + pow(BallOffsetSideNow, 2));
			float NormObjVectY = BallOffsetForNow / sqrt(pow(BallOffsetForNow, 2) + pow(BallOffsetSideNow, 2));
			float DotProd = NormObjVectX * sin(BallOffsetRotNow) + NormObjVectY * cos(BallOffsetRotNow);

			// Apply the movement
			ModelMatrix =
				glm::rotate(identity, BallOffsetRotNow, glm::vec3(0.0f, 0.0f, 1.0f)) *
				glm::translate(identity, glm::vec3(-BallOffsetSideNow, -BallOffsetForNow, 0.0f));
			glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix));
			
			if (closed)
			{
				// Draw the shapes
				//glBindVertexArray(vao[0]);
				//glDrawArrays(GL_TRIANGLES, 0, vertices_obj1.size());
				glBindVertexArray(vao[1]);
				glDrawArrays(GL_TRIANGLES, 0, vertices_obj2.size());
				//glBindVertexArray(vao[2]);
				//glDrawArrays(GL_TRIANGLES, 0, vertices_obj.size());
			}
			else
			{
				glBindVertexArray(vao[3]);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}

			// Unused code for drawing other shapes
			if (0){
				// Grating
				glBindVertexArray(vao[2]);
				int numStripes = 18;
				float gratAng = 360.0 / 18.0;
				for (int angGrat = 0; angGrat < numStripes; angGrat++)
				{
					ModelMatrix =
						//glm::translate(identity, glm::vec3(dist2stripe*sinf((BallOffsetRotNow) * M_PI / 180), dist2stripe*(1.0f - cosf((BallOffsetRotNow)  * M_PI / 180)), 0.0f)) *
						glm::rotate(identity, BallOffsetRotNow + gratAng*angGrat, glm::vec3(0.0f, 0.0f, 1.0f));
					glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix));
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				}
				// Second stripe
				ModelMatrix =
					glm::rotate(identity, BallOffsetRotNow, glm::vec3(0.0f, 0.0f, 1.0f)) *
					glm::translate(identity, glm::vec3(-BallOffsetSideNow, -BallOffsetForNow, 0.0f)) *
					glm::rotate(identity, 180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
				glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


				for (int ang = 0; ang < int(numAng); ang++) {
					// Ground plane
					ModelMatrix =
						glm::rotate(identity, BallOffsetRotNow, glm::vec3(0.0f, 0.0f, 1.0f)) *
						glm::translate(identity, glm::vec3(-BallOffsetSideNow, -BallOffsetForNow, 0.0f)) *
						glm::rotate(identity, (float) 360.0f / numAng*ang, glm::vec3(0.0f, 0.0f, 1.0f));
					glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix));
					glBindTexture(GL_TEXTURE_2D, tex[2]);
					glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(18 * sizeof(GLfloat)));
					glBindTexture(GL_TEXTURE_2D, tex[n]);

					// Trench
					//glBindTexture(GL_TEXTURE_2D, tex[2]);
					//glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, (void*)(27 * sizeof(GLfloat)));
					//glBindTexture(GL_TEXTURE_2D, tex[n]);
				}
			}
		}
		// Capture the stripe as a texture
		glBindTexture(GL_TEXTURE_2D, tex[1 + n]);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, SCRWIDTH, SCRHEIGHT, 0);
	}


	// Map the textures onto a rectangle/window that spans all three projectors and apply the appropriate shader distortion corrections
	{
		// Allow the different colors/textures to be blended
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		// Clear the screen and apply
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(vao[3]);

		glUniform1f(setColor, (int)0);
		glUniform1f(cylLocation, (float) 1.0f); // Allow the projection to be distorted for the cylindrical screen using the shader
		glViewport(0, 0, SCRWIDTH, SCRHEIGHT); // Open up the viewport to the full screen
		ViewMatrix = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, dist2stripe, 0), glm::vec3(0, 0, 1)); //Look at the center of the rectangle
		glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(ViewMatrix));
		glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(identity));
		glUniform1f(ProjNumber, (int)1);  // Correct for the brightness difference between projectors

		// Draw the rectangle
		for (int n = 0; n < 3; n++) {
			glBindTexture(GL_TEXTURE_2D, tex[1+n]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLfloat)));
		}

	}

	if (direction == 0)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw a box to trigger the photodiode
		glUniform1f(cylLocation, (float) 0.0f); // Initially, we want an undistorted projection
		glUniform1f(ProjNumber, (int)100);  // No brightness correction the first time
		glBindTexture(GL_TEXTURE_2D, tex[0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(12 * sizeof(GLfloat)));

}

// Load a Blender generated object
bool loadOBJ( const char * path, std::vector<glm::vec3> & out_vertices, std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals)
{
	// Indices of the vertices, texture coordinates, and normals
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	// Temporary vectors to store the values from each line
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	// Open the file
	FILE * objFile;
	fopen_s(&objFile, path, "r");
	// Pull the data out of the file
	while (1){

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf_s(objFile, "%s", lineHeader, _countof(lineHeader));
		if (res == EOF){
			break; // EOF = End Of File. Quit the loop.
		}
		// parse lineHeader
		if (strcmp(lineHeader, "v") == 0){
			glm::vec3 vertex;
			fscanf_s(objFile, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0){
			glm::vec2 uv;
			fscanf_s(objFile, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0){
			glm::vec3 normal;
			fscanf_s(objFile, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0){
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf_s(objFile, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9){
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
		else{
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, objFile);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i<vertexIndices.size(); i++){

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}

	return true;
}


// Clear the buffers on shutdown
void GLShutdown(void)
{
	glDeleteTextures(1, tex);
	glDeleteProgram(shader_program);
	glDeleteShader(fs);
	glDeleteShader(vs);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao[0]);
	glDeleteVertexArrays(1, &vao[1]);
	CloseOffset();
}