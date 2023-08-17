#include <iostream>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"      // Image loading Utility functions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "meshes.h"
#include "Cam.h"



using namespace std;

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif
namespace
{
	const char* const WINDOW_TITLE = "Project Work";
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	/// Main GLFW Window
	GLFWwindow* gWindow = nullptr;
	GLuint gProgramId;
	Meshes meshes;
	//Camera
	Camera gCamera(glm::vec3(0.0f, 8.0f, 3.0f));

	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	float gCameraSpeed = 2.5f;
	bool gFirstMouse = true;
	glm::vec3 gCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 gCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 gCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 gCameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
	//timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
	};



	// Triangle mesh data
	GLMesh gMesh;
	// Texture id
	GLuint gTextureIdKeyboard;
	GLuint gTextureIdDesk;
	GLuint gTextureIdKeys;
	bool gIsHatOn = true;


}


// variable to handle ortho change
bool perspective = false;

const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

out vec4 vertexColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f);
	vertexColor = color;
}
);

const GLchar* fragmentShaderSource = GLSL(440,
	in vec4 vertexColor;
out vec4 fragmentColor;
uniform vec4 uObjectColor;

void main()
{
	fragmentColor = uObjectColor;

}
);


bool UInitialize(int argc, char* argv[], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;
	// Set the mouse position callback
	glfwSetCursorPosCallback(gWindow, UMousePositionCallback);

	// Set the mouse scroll callback
	glfwSetScrollCallback(gWindow, UMouseScrollCallback);
	glfwSetInputMode(gWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
	meshes.CreateMeshes();

	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
		return EXIT_FAILURE;
	// Load textures

	// Load textures
	const char* texFilename = "Textures/keyboard.jpg";  // Keyboardtexture

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdKeyboard);

	texFilename = "Textures/keys.jpg"; // keys texture

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gTextureIdKeys);


	// Load textures
	texFilename = "Textures/desktop.jpg";  // Desk texture

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gTextureIdDesk); // Desk texture





	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	while (!glfwWindowShouldClose(gWindow))
	{
		UProcessInput(gWindow);

		// per-frame timing
	   // --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		//camera/view tranformation
		glm::mat4 view = glm::mat4(1.0f); //initializing matirx
		float radius = 10.0f;
		float camx = sin(glfwGetTime()) * radius;
		float camz = cos(glfwGetTime()) * radius;
		view = glm::lookAt(glm::vec3(camx, 0.0f, camz), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		URender();
		glfwPollEvents();
	}

	meshes.DestroyMeshes();
	//destroying textures
	UDestroyTexture(gTextureIdKeys);
	UDestroyTexture(gTextureIdDesk);
	UDestroyTexture(gTextureIdKeyboard);
	UDestroyShaderProgram(gProgramId);

	glfwTerminate();
	return EXIT_SUCCESS;
}


bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);


	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
	if (*window == nullptr)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		glfwTerminate();
		return false;
	}

	if (glfwGetCurrentContext() == nullptr)
	{
		std::cerr << "Failed to create OpenGL context" << std::endl;
		glfwTerminate();
		return false;
	}

	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
	return true;

}



void UProcessInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	bool perspective = false;
	// WASD


	float cameraOffset = gCameraSpeed * gDeltaTime;

	if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessInput(FORWARD, cameraOffset);
	if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessInput(BACKWARD, cameraOffset);
	if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessInput(LEFT, cameraOffset);
	if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessInput(RIGHT, cameraOffset);
	if (glfwGetKey(gWindow, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessInput(UP, cameraOffset);
	if (glfwGetKey(gWindow, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessInput(DOWN, cameraOffset);

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		perspective = false;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		perspective = true;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0.0)
		gCameraSpeed *= 2.1f;
	else if (yoffset < 0.0)
		gCameraSpeed /= 1.1f;
}





void URender()
{
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 translation;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint objectColorLoc;
	// Enable z-depth
	glEnable(GL_DEPTH_TEST);
	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// camera/view transformation
	view = glm::lookAt(gCameraPos, gCameraPos + gCameraFront, gCameraUp);

	// Creates an orthographic or perspective projection
	projection;
	if (!perspective) {
		view = gCamera.GetViewMatrix();
		projection = glm::perspective(glm::radians(60.0f), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else {
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}

	// Set the shader to be used
	glUseProgram(gProgramId);

	// Set the shader to be used
	glUseProgram(gProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");
	objectColorLoc = glGetUniformLocation(gProgramId, "uObjectColor");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));



	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(.2f, 5.5f, .2f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(0.0, 0.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(2.75f, 3.0f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
	glUniform1i(glGetUniformLocation(gProgramId, "bHasTexture"), 1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	glBindVertexArray(0);


	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(8.0f, 8.0f, 8.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 2.5f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 2);
	glUniform1i(glGetUniformLocation(gProgramId, "bHasTexture"), 1);

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(8.0f, 0.1f, 2.0f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(15.0f), glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, 3.0f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);
	glUniform1i(glGetUniformLocation(gProgramId, "bHasTexture"), 1);

	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);


	glBindVertexArray(0);


	glBindVertexArray(meshes.gBoxMesh.vao);
	// Define the number of keys in each row and column
	int numKeysX = 15; // Number of keys across
	int numKeysZ = 4; // Number of keys down

	// Define the spacing between keys
	float spacingX = 0.5f;
	float spacingZ = 0.5f;

	// Define the position of the board
	glm::vec3 boardPosition(0.0f, 3.0f, 0.0f);




	// Define the starting position of the keys relative to the board
	glm::vec3 startPos(-3.50f, 0.25f, 1.0f);
	for (int z = 0; z < numKeysZ; ++z)
	{
		for (int x = 0; x < numKeysX; ++x)
		{
			// Calculate the position for each key based on the row and column index
			float posX = boardPosition.x + startPos.x + x * spacingX;
			float posY = boardPosition.y + startPos.y;
			float posZ = boardPosition.z + startPos.z - z * spacingZ;

			// 1. Scales the object (key)
			scale = glm::scale(glm::vec3(.25f, .25f, .25f));


			// 3. Position the object
			translation = glm::translate(glm::vec3(posX, posY, posZ));
			// Model matrix: transformations are applied right-to-left order
			model = translation * rotation * scale;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 1);
			glUniform1i(glGetUniformLocation(gProgramId, "bHasTexture"), 1);

			glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
		}
	}


	glBindVertexArray(0);



	glfwSwapBuffers(gWindow);
}

bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	int success = 0;
	char infoLog[512];

	programId = glCreateProgram();
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShaderId, 1, &vtxShaderSource, nullptr);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, nullptr);

	glCompileShader(vertexShaderId);
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, nullptr, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		glDeleteShader(vertexShaderId); // Delete the shader object
		return false;
	}

	glCompileShader(fragmentShaderId);
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), nullptr, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		glDeleteShader(vertexShaderId);   // Delete the shader objects
		glDeleteShader(fragmentShaderId);
		return false;
	}

	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), nullptr, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		glDeleteShader(vertexShaderId);   // Delete the shader objects
		glDeleteShader(fragmentShaderId);
		return false;
	}

	glDetachShader(programId, vertexShaderId);   // Detach the shader objects
	glDetachShader(programId, fragmentShaderId);
	glDeleteShader(vertexShaderId);   // Delete the shader objects
	glDeleteShader(fragmentShaderId);

	glUseProgram(programId);
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");


	return true;
}

void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}
/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{


		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}
void UDestroyTexture(GLuint textureId)
{
	glDeleteTextures(1, &textureId);
}