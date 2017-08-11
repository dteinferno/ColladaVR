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
#include "colladainterface.h"

// Specify whether the stripe is continuous (0) or jumps across the back (1)
int jump = 0;

// Define variable for correlated noise
const int numObjects = 100;
int objState[numObjects] = { 0 }; // the state of the object
float objRot[numObjects] = { 0.0f }; // the rotation offset of the object
float objZ[numObjects] = { 0.0f }; // the z offset of the object
float objZInit[numObjects] = { 0.0f }; // the z position of the object
float objFlipT[numObjects] = { 0.0f }; // the next time for the object to flip
int objFlipPt[numObjects] = { 0 }; // the flip time for the object to take

const int numFlips = 250;
float flipTimes[numObjects][numFlips]; //the list of times when the object flips its state
float rotList[numObjects*numFlips] = { 0.0f }; // the list of random rotations to draw from
int rotIndex = 0; // index to keep track of where I'm drawing the rotation from
float zList[numObjects*numFlips] = { 0.0f }; // the list of random z shifts to draw from
int zIndex = 0; // index to keep track of where I'm drawing the z value
float zMin = -10.0f; //The minimum z value
float zMax = 30.0f; // The maximum z value
float zRange = zMax - zMin;

// Define constants related to the projector angles
float dist2stripe = 20;
float fovAng = 80 * M_PI / 180;
float numDivAngs = 5*3; // usually 5 * 3
float fovAngNow = fovAng * 3 / numDivAngs;

// Define offset values (Rotational, Forward, and Lateral)
float BallOffsetRotNow = 0.0f;
float BallOffsetForNow = 0.0f;
float BallOffsetSideNow = 0.0f;
float RotOffset = 0.0f;
float RotOffsetTemp = 0.0f;

// Variable to control the direction of the open loop stripe (+/-1) AND whether or not to display anything at all (0)
int olsdir;

// Variable to set the rate for open loop
float gain;

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
"uniform sampler2D tex;"
"void main () {"

"  float LtoScreen = 6.55;"
"  float wofScreen = 1.93;"

"  float angofScreen = 4*3.141592/9;"

"  float texshifts = wofScreen*(3*Texcoord.s-1.5);"
" if (Texcoord.s < 0.3333)"
"  texshifts = wofScreen*(3*Texcoord.s-0.5);"
" if (Texcoord.s > 0.6666)"
"  texshifts = wofScreen*(3*Texcoord.s-2.5);"

"  float texshiftt = Texcoord.t-0.5;"
"  float modtfactor = (1.5 - sqrt(1.5*1.5-texshifts*texshifts))/LtoScreen + 1.0;"
"  float distortt = texshiftt*modtfactor + 0.5;"

"  float texchop = 3*Texcoord.s;"
"  float distorts = 0.3881*pow(texchop,4)-0.1844*pow(texchop,3) - 0.2624*pow(texchop,2)+1.049*texchop;"
" if (Texcoord.s > 0.3333)"
" {"
"  texchop = texchop - 1;"
"  distorts = 0.3881*pow(texchop, 4) - 0.1844*pow(texchop, 3) - 0.2624*pow(texchop, 2) + 1.049*texchop;"
" }"
" if (Texcoord.s > 0.6666)"
" {"
"  texchop = texchop - 1;"
//"  distorts = 0.3881*pow(texchop, 4) - 0.1844*pow(texchop, 3) - 0.2624*pow(texchop, 2) + 1.049*texchop;"
"  distorts = 0.6128*pow(texchop, 4) - 0.474*pow(texchop, 3) - 0.1926*pow(texchop, 2) + 1.037*texchop;"
" }"

"  distorts = 0.333 * distorts; "
" if (Texcoord.s > 0.3333)"
"  distorts = distorts + 0.333;"
" if (Texcoord.s > 0.6666)"
"  distorts = distorts + 0.333;"

" float brightcorrect = 2.285*pow(distorts, 4) - 4.57*pow(distorts, 3) + 4.496*pow(distorts, 2) - 2.211*distorts + 0.9976; "

"  if (cyl == 0.0)"
"  {"
"   distorts = Texcoord.s;"
"   distortt = Texcoord.t;"
"   brightcorrect = 1.0;"
"  }"

"  vec2 distort = vec2 (distorts, distortt);"
"  vec4 unmodColor = texture(tex, distort);"
" if (color == 1)"
" {"
"  unmodColor.r = 0.0;"
"  unmodColor.g = 0.0;"
" }"
" if (color == 2)"
" {"
"  unmodColor.g = 0.0;"
"  unmodColor.b = 0.0;"
" }"
" if (color == 3)"
" {"
"  unmodColor.r = 0.0;"
"  unmodColor.b = 0.0;"
" }"
"  frag_colour = brightcorrect*vec4(unmodColor.r*1.0, unmodColor.g*1.0, unmodColor.b*1.0, 1.0 );"
"}";

// Define the OpenGL constants
std::vector<ColGeom> geom_vec;    // Vector containing COLLADA meshes
int num_objects;                  // Number of meshes in the vector
std::vector<ColTrans> trans_vec; // Vector containing COLLADA transformations
std::vector<ColTex> tex_vec;    // Vector containing COLLADA textures
int num_images;                  // Number of images in the vector
std::vector<ColIm> im_vec;    // Vector containing COLLADA texture images
GLuint *vaos, *vbos, *ebos;              // OpenGL vertex objects
GLuint vs;
GLuint fs;
GLuint shader_program;
GLuint *tex;
GLuint cylLocation;
GLuint setColor;
glm::mat4 ProjectionMatrix;
GLuint ProjectionID;
glm::mat4 ViewMatrix;
GLuint ViewID;
glm::mat4 ModelMatrix;
GLuint ModelID;
glm::mat4 TransMatrix[numObjects];
glm::mat4 ModMatrixObj[numObjects];
int objTexID[numObjects];
glm::mat4 identity;

// InitOpenGL: initializes OpenGL; defines buffers, constants, etc...
void InitOpenGL(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);


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

	// Pointers to the shader position, normal, and texture variables
	GLint posAttrib;
	GLint normAttrib;
	GLint texAttrib;

	// Initialize COLLADA geometries
	ColladaInterface::readGeometries(&geom_vec, ColladaFname);
	num_objects = (int)geom_vec.size();

	// Create a VAO for each geometry
	vaos = new GLuint[num_objects+1];
	glGenVertexArrays(num_objects+1, vaos);

	// Create a VBO for each geometry - one each for vertices, normals, and texture coordinates
	vbos = new GLuint[3 * num_objects + 1];
	glGenBuffers(3 * num_objects + 1, vbos);

	// Initialize the element array buffer
	ebos = new GLuint[num_objects + 1];;
	glGenBuffers(num_objects + 1, ebos);

	// Check if the object loaded properly
	bool resdat;
	

	// Configure VBOs to hold positions, texture coordinates, and normals for each geometry
	for (int i = 0; i < num_objects; i++) {

		// Vectors to hold the info
		std::vector<glm::vec3> vertices_obj;
		std::vector<glm::vec2> uvs_obj;
		std::vector<glm::vec3> normals_obj;

		glBindVertexArray(vaos[i]);

		// Convert the maps from the Collada file to vectors for OpenGL
		resdat = loadOBJ(geom_vec[i].map["VERTEX"], geom_vec[i].map["NORMAL"], geom_vec[i].map["TEXCOORD"], geom_vec[i].index_count, geom_vec[i].indices, vertices_obj, uvs_obj, normals_obj);

		// Load the data to the shaders 
		glBindBuffer(GL_ARRAY_BUFFER, vbos[3*i]);
		glBufferData(GL_ARRAY_BUFFER, vertices_obj.size() * sizeof(glm::vec3), &vertices_obj[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[3*i+1]);
		glBufferData(GL_ARRAY_BUFFER, uvs_obj.size() * sizeof(glm::vec2), &uvs_obj[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[3*i+2]);
		glBufferData(GL_ARRAY_BUFFER, normals_obj.size() * sizeof(glm::vec3), &normals_obj[0], GL_STATIC_DRAW);

		// Create a pointer for the position
		glBindBuffer(GL_ARRAY_BUFFER, vbos[3*i]);
		posAttrib = glGetAttribLocation(shader_program, "position");
		glEnableVertexAttribArray(posAttrib);
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Create a pointer for the texture coordinates
		glBindBuffer(GL_ARRAY_BUFFER, vbos[3*i+1]);
		texAttrib = glGetAttribLocation(shader_program, "texcoord");
		glEnableVertexAttribArray(texAttrib);
		glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}

	// Get out the Collada transformations
	ColladaInterface::readTransformations(&trans_vec, ColladaFname);

	// Create our distorted screen
	// Initialize the vertex array object
	glBindVertexArray(vaos[num_objects]);

	// Define constants for defining the shapes
	float windowSpan =  dist2stripe * tanf(fovAng / 2);
	float aspect = float(SCRWIDTH) / float(SCRHEIGHT);

	// Define the vertices
	float vertices[] = {
		// The first four points are the position, while the final two are the texture coordinates
		// First set of points for the final display that the texture will be mapped onto.
		-windowSpan, (float)dist2stripe, -windowSpan / aspect, 1.0f, 0.0f, 1.0f,
		-windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 0.0f, 0.0f,
		windowSpan, (float)dist2stripe, -windowSpan / aspect, 1.0f, 1.0f, 1.0f,
		windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 1.0f, 0.0f,

		// Second set of points for a blinking dot that will trigger the photodiode.
		windowSpan, (float)dist2stripe, windowSpan * (1 / aspect - 0.15), 1.0f, 1.0f, 1.0f,
		windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 1.0f, 0.0f,
		0.85*windowSpan, (float)dist2stripe, windowSpan * (1 / aspect - 0.15), 1.0f, 0.0f, 1.0f,
		0.85*windowSpan, (float)dist2stripe, windowSpan / aspect, 1.0f, 0.0f, 0.0f,
	};

	// Bind the vertex data to the buffer array
	glBindBuffer(GL_ARRAY_BUFFER, vbos[3 * num_objects]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Define the elements for the stripe and the projected screen.
	GLuint elements[] = {
		2, 1, 0,
		1, 2, 3,

		4, 5, 6,
		7, 6, 5,
	};

	//Bind the element data to the array.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebos[num_objects + 1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// Create a pointer for the position
	posAttrib = glGetAttribLocation(shader_program, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

	// Create a pointer for the texture coordinates
	texAttrib = glGetAttribLocation(shader_program, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));

	// Initialize COLLADA textures
	ColladaInterface::readTextures(&tex_vec, ColladaFname);
	num_images = (int)tex_vec.size();

	// Create a texture array
	tex = new GLuint[num_images+5];
	glGenTextures(num_images+5, tex);

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

	// Uniform black texture
	glBindTexture(GL_TEXTURE_2D, tex[1]);
	float pixelsB[] = {
		0.0f, 0.0f, 0.0f,
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, pixelsB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	// Prep the remaining textures to be used for cylindrical distortion
	for (int n = 0; n < 3; n++) {
		glBindTexture(GL_TEXTURE_2D, tex[2 + n]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// Generate textures from Collada file
	ColladaInterface::readImages(&im_vec, ColladaFname);
	for (int texs = 0; texs < num_images; texs++)
	{
		if (tex_vec[texs].texfname != "")
		{
			for (int ims = 0; ims < im_vec.size(); ims++)
			{
				if (tex_vec[texs].texfname == im_vec[ims].imagename)
				{
					// Load a texture using the SOIL loader
					int texWidth, texHeight;
					char texfname[100];
					const char *  texfpath = "D:\\Environments\\";
					unsigned char* texImage;
					FILE *fnstr;
					strncpy_s(texfname, 100, texfpath, strlen(texfpath));
					strcat_s(texfname, 100, im_vec[ims].imageloc.c_str());
					texImage = SOIL_load_image(texfname, &texWidth, &texHeight, 0, SOIL_LOAD_RGB);
					glBindTexture(GL_TEXTURE_2D, tex[5 + texs]);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, texImage);
					SOIL_free_image_data(texImage);
				}
			}
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, tex[5 + texs]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, tex_vec[texs].RGB);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	// Pull out the cylindrical distortion switch uniform from the shader
	cylLocation = glGetUniformLocation(shader_program, "cyl");

	// Pull out the color setting
	setColor = glGetUniformLocation(shader_program, "color");

	// Pull out the matrices
	ProjectionID = glGetUniformLocation(shader_program, "Projection");
	ViewID = glGetUniformLocation(shader_program, "View");
	ModelID = glGetUniformLocation(shader_program, "Model");

	// Populate the random white noise arrays

	//Read in the initial states
	std::string strState;
	std::ifstream startStates("D://Environments//startState.txt");

	for (int stateInit = 0; stateInit < numObjects; stateInit++)
	{
		std::getline(startStates, strState);
		std::stringstream ss(strState);
		ss >> objState[stateInit];
	}

	// Read in the times when the object should flip states
	std::ifstream flipTs("D://Environments//flipTimes.txt");
	std::stringstream ss;
	std::string objFlips;

	for (int numObjs = 0; numObjs < numObjects; numObjs++)
	{
		std::getline(flipTs, objFlips);
		ss << objFlips;
		for (int flipNum = 0; flipNum < numFlips; flipNum++)
		{
			std::string objFlipVal;
			std::getline(ss, objFlipVal, ',');
			flipTimes[numObjs][flipNum] = atof(objFlipVal.c_str());
			if (flipNum == 0)
				objFlipT[numObjs] = flipTimes[numObjs][flipNum];
		}	
		ss << "";
		ss.clear();
	}

	//Read in the rotational and z shifts
	std::ifstream flipRotZ("D://Environments//flipValues.txt");
	std::string flipVals;

	for (int rotZ = 0; rotZ < 2; rotZ++)
	{
		std::getline(flipRotZ, flipVals);
		ss << flipVals;
		for (int getRotZ = 0; getRotZ < numObjects*numFlips; getRotZ++)
		{
			std::string objRotZ;
			std::getline(ss, objRotZ, ',');
			if (rotZ == 0)
				rotList[getRotZ] = atof(objRotZ.c_str());
			if (rotZ == 1)
				zList[getRotZ] = atof(objRotZ.c_str());
		}
		ss << "";
		ss.clear();
	}

	// Calculate the base translation matrices, find the texture ID for each object, and set the z position for each object
	for (int i = 0; i<num_objects; i++) {
		// Get the scaling and translation for the object
		for (int ts = 0; ts < num_objects; ts++) {
			if (trans_vec[ts].name == geom_vec[i].name){
				TransMatrix[i] =
					glm::rotate(identity, 180.0f, glm::vec3(1.0f, 0.0f, 0.0f)) *
					glm::translate(identity, glm::vec3(trans_vec[ts].trans_data[0], trans_vec[ts].trans_data[1], trans_vec[ts].trans_data[2])) *
					glm::rotate(identity, trans_vec[ts].rotZ_data[3], glm::vec3(trans_vec[ts].rotZ_data[0], trans_vec[ts].rotZ_data[1], trans_vec[ts].rotZ_data[2])) *
					glm::rotate(identity, trans_vec[ts].rotY_data[3], glm::vec3(trans_vec[ts].rotY_data[0], trans_vec[ts].rotY_data[1], trans_vec[ts].rotY_data[2])) *
					glm::rotate(identity, trans_vec[ts].rotX_data[3], glm::vec3(trans_vec[ts].rotX_data[0], trans_vec[ts].rotX_data[1], trans_vec[ts].rotX_data[2])) *
					glm::scale(identity, glm::vec3(trans_vec[ts].scale_data[0], trans_vec[ts].scale_data[1], trans_vec[ts].scale_data[2]));
			}
		}
		// Find the texture for each object
		for (int ims = 0; ims < num_images; ims++)
		{
			if (geom_vec[i].texture == tex_vec[ims].name)
			{
				objTexID[i] = 5 + ims;	
			}
		}

		objZInit[i] = trans_vec[i].trans_data[2]; 
	}	

	//Create a file to store the offset position at each point in time
	//FILE *strTest;
	//fopen_s(&strTest, "test.txt", "w");
	//for (int i = 0; i < num_objects; i++) {
	//	fprintf(strTest, "%f,", objZInit[i]);
	//}
	//fclose(strTest);

}

void RenderFrame(int closed, int trans, int direction, float lookDownAng, float gain, float timeStart, float timeNow, float clGain)
{
	//Check to see if the object has flipped and update random offset values.
	for (int i = 0; i < num_objects; i++) {
		if (objFlipT[i] < timeNow)
		{
			objFlipPt[i] = objFlipPt[i] + 1;
			objFlipT[i] = flipTimes[i][objFlipPt[i]];
			objState[i] = ~objState[i];
			objRot[i] = rotList[rotIndex];
			rotIndex++;
			//if (zMax > (objZInit[i] + zList[zIndex]))
			//	objZ[i] = -zList[zIndex];
			//else
			//	objZ[i] = zRange-zList[zIndex];
			zIndex++;
			ModMatrixObj[i] = glm::translate(identity, glm::vec3(0.0f, 0.0f, objZ[i]))*glm::rotate(identity, objRot[i], glm::vec3(0.0f, 0.0f, 1.0f)) * TransMatrix[i];
		}
		ModMatrixObj[i] = TransMatrix[i];
	}

	ProjectionMatrix = glm::perspective(float(360 / M_PI * atanf(tanf(fovAngNow / 2) * float(SCRHEIGHT / 2) / float(SCRWIDTH * 3 / numDivAngs))), float(SCRWIDTH * 3 / numDivAngs) / float(SCRHEIGHT / 2), 0.1f, 1000.0f); //Set the perspective for the projector
	glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(ProjectionMatrix));
	// Map the desired image onto the three different colors
	for (int n = 0; n < 3; n++) {

		//Clear the image and bind the appropriate color texture
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set the background color to black
		glClearDepth(1.0f);
		glUniform1i(setColor, (int)n+1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers
		glUniform1f(cylLocation, (float) 0.0f); // Initially, we want an undistorted projection

		// Take a picture for each of the camera angles
		for (int windowNum = 0; windowNum < numDivAngs; windowNum++) {

			// Define the scene to be captured
			glViewport(SCRWIDTH*windowNum / numDivAngs, SCRHEIGHT / 4, SCRWIDTH / numDivAngs, SCRHEIGHT / 2 ); // Restrict the viewport to the region of interest
			float angNow = fovAngNow * (windowNum - (numDivAngs - 1)/2);
			ViewMatrix = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(sinf(angNow), cosf(angNow), 0), glm::vec3(0, 0, 1)) * glm::rotate(identity, lookDownAng, glm::vec3(1.0f, 0.0f, 0.0f)); //Look at the appropriate direction for the projector
			glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(ViewMatrix));

			if (closed) // Advance the environment according to the ball movement
			{
				io_mutex.lock();
				BallOffsetForNow =  BallOffsetFor;
				BallOffsetSideNow = BallOffsetSide;
				if (trans == 2)
				{
					TimeOffset(BallOffsetRotNow, direction, timeStart, gain);
					RotOffset = 2*BallOffsetRotNow-RotOffset;
				}
				if (trans == 3)
				{
					RotOffset = 60;
				}
				if (trans == 4)
				{
					RotOffset = 0;
				}
				if (clGain >= 0)
				{
					BallOffsetRotNow = clGain*BallOffsetRot + RotOffset;
				}
				else
				{
					BallOffsetRotNow = clGain*BallOffsetRot;
				}

				io_mutex.unlock();

			}
			else  // Advance the environment according to open loop parameters
			{
				if (trans == 1)
				{
					TimeOffset(BallOffsetForNow, direction, timeStart, gain);
					BallOffsetRotNow = 0.0f;
				}
				if (trans == 0)
				{
					TimeOffset(BallOffsetRotNow, direction, timeStart, gain);
					BallOffsetForNow = 0.0f;
				}
				BallOffsetSideNow = 0.0f;
			}

			int intBOR = int(BallOffsetRotNow) + 180;
			if (jump)
			{
				if (intBOR % 360 > 300)
				{
					BallOffsetRotNow = BallOffsetRotNow - 240.0f;
					io_mutex.lock();
					BallOffsetRot = BallOffsetRot - 240.0f/clGain;
					io_mutex.unlock();

				}
				if (intBOR % 360 < 60)
				{
					BallOffsetRotNow = 240.0f + BallOffsetRotNow;
					io_mutex.lock();
					BallOffsetRot = BallOffsetRot + 240.0f/clGain;
					io_mutex.unlock();
				}
			}

			ModelMatrix =
				glm::rotate(identity, BallOffsetRotNow, glm::vec3(0.0f, 0.0f, 1.0f)) *
				glm::translate(identity, glm::vec3(-BallOffsetSideNow, -BallOffsetForNow, 0.0f));
			
			// Draw each object
			for (int i = 0; i<num_objects; i++) {
				glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix*ModMatrixObj[i]));
				// Flip objects off and on
				//if (objState[i])
					glBindTexture(GL_TEXTURE_2D, tex[objTexID[i]]); // Bind the appropriate texture
				//else
				//	glBindTexture(GL_TEXTURE_2D, tex[1]);
				glBindVertexArray(vaos[i]);
				glDrawArrays(GL_TRIANGLES, 0, geom_vec[i].index_count);
			}
		}
		// Capture the stripe as a texture
		glBindTexture(GL_TEXTURE_2D, tex[2 + n]);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, SCRWIDTH, SCRHEIGHT, 0);
	}


	// Map the textures onto a rectangle/window that spans all three projectors and apply the appropriate shader distortion corrections
	{
		// Allow the different colors/textures to be blended
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		// Clear the screen and apply
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(vaos[num_objects]);

		glUniform1i(setColor, (int)0);
		glUniform1f(cylLocation, (float) 1.0f); // Allow the projection to be distorted for the cylindrical screen using the shader
		glViewport(0, 0, SCRWIDTH, SCRHEIGHT); // Open up the viewport to the full screen
		ProjectionMatrix = glm::perspective(float(360 / M_PI * atanf(tanf(fovAng / 2) * float(SCRHEIGHT) / float(SCRWIDTH))), float(SCRWIDTH) / float(SCRHEIGHT), 0.1f, 1000.0f); //Set the perspective for the projector
		ViewMatrix = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, dist2stripe, 0), glm::vec3(0, 0, 1)); //Look at the center of the rectangle
		glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(ProjectionMatrix));
		glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(ViewMatrix));
		glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(identity));

		// Draw the rectangle
		for (int n = 0; n < 3; n++) {
			glBindTexture(GL_TEXTURE_2D, tex[2+n]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
		}

	}

	if (direction == 0)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void PDBox(void)
{
	// Draw a box to trigger the photodiode
	glUniform1f(cylLocation, (float) 0.0f); // Initially, we want an undistorted projection
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLfloat)));
}

// Convert map data from a Collada file to vector data
bool loadOBJ(SourceData Vertex, SourceData Normal, SourceData Texcoord, int numIndices, unsigned short* Indices, std::vector<glm::vec3> & out_vertices, std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals)
{
	// Indices of the vertices, texture coordinates, and normals
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;

	// Temporary vectors to store the values from each line
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	// How to increment the indices
	int incInd;

	// Get out the vertex data
	float * vertexData = (float *) malloc(Vertex.size);
	vertexData = (float *) Vertex.data;
	for (int vertStep = 0; vertStep < Vertex.size/ 3 / sizeof(float); vertStep++)
	{
		glm::vec3 vertex;
		vertex.x = vertexData[3 * vertStep];
		vertex.y = vertexData[3 * vertStep + 1];
		vertex.z = vertexData[3 * vertStep + 2];
		temp_vertices.push_back(vertex);
	}

	// Get out the texture data if it exists. Otherwise set arbitrary texture coordinates.
	if (Texcoord.size > 0)
	{
		float * textureData = (float *)malloc(Texcoord.size);
		textureData = (float *)Texcoord.data;
		for (int texStep = 0; texStep < Texcoord.size / 2 / sizeof(float); texStep++)
		{
			glm::vec2 uv;
			uv.x = textureData[2 * texStep];
			uv.y = textureData[2 * texStep + 1];
			temp_uvs.push_back(uv);
		}
		incInd = 3;
	}
	else
	{
		glm::vec2 uv;
		uv.x = 0.0f;
		uv.y = 0.0f;
		temp_uvs.push_back(uv);
		incInd = 2;
	}

	// Get out the normal data
	float * normalData = (float *)malloc(Normal.size);
	normalData = (float *)Normal.data;
	for (int normStep = 0; normStep < Normal.size / 3 / sizeof(float); normStep++)
	{
		glm::vec3 normal;
		normal.x = normalData[3 * normStep];
		normal.y = normalData[3 * normStep + 1];
		normal.z = normalData[3 * normStep + 2];
		temp_normals.push_back(normal);
	}

	// Sort the indices
	for (int indexStep = 0; indexStep < numIndices / 3; indexStep++)
	{
		std::string vertex1, vertex2, vertex3;
		unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];

		vertexIndex[0] = Indices[incInd * 3 * indexStep];
		normalIndex[0] = Indices[incInd * 3 * indexStep + 1];
		vertexIndex[1] = Indices[incInd * 3 * indexStep + incInd];
		normalIndex[1] = Indices[incInd * 3 * indexStep + incInd + 1];
		vertexIndex[2] = Indices[incInd * 3 * indexStep + incInd * 2];
		normalIndex[2] = Indices[incInd * 3 * indexStep + incInd * 2 + 1];

		if (Texcoord.size > 0)
		{
			uvIndex[0] = Indices[incInd * 3 * indexStep + 2];
			uvIndex[1] = Indices[incInd * 3 * indexStep + incInd + 2];
			uvIndex[2] = Indices[incInd * 3 * indexStep + incInd * 2 + 2];
		}
		else
		{
			uvIndex[0] = 0;
			uvIndex[1] = 0;
			uvIndex[2] = 0;
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


	// For each vertex of each triangle
	for (unsigned int i = 0; i<vertexIndices.size(); i++)
	{
		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex];
		glm::vec2 uv = temp_uvs[uvIndex];
		glm::vec3 normal = temp_normals[normalIndex];

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
	glDeleteTextures(num_images+5, tex);
	glDeleteProgram(shader_program);
	glDeleteShader(fs);
	glDeleteShader(vs);

	// Deallocate mesh data
	//ColladaInterface::freeGeometries(&geom_vec);

	// Deallocate OpenGL objects
	glDeleteBuffers(3 * num_objects + 1, vbos);
	glDeleteBuffers(num_objects + 1, vaos);
	delete(vbos);
	delete(vaos);

	CloseOffset();
}