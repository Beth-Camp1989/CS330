#include <iostream>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"     // Image loading Utility functions
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
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	float gCameraSpeed = 1.0f;
	bool gFirstMouse = true;
	//timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
	};
	/// Main GLFW Window
	GLFWwindow* gWindow = nullptr;
	// Triangle mesh data
	GLMesh gMesh;
	// Texture id
	GLuint gTextureIdKeyboard;
	GLuint gTextureIdDesk;
	GLuint gTextureIdBoxes;
	GLuint gTextureIdNotebook;
	GLuint gTextureIdPhone;
	GLuint gTextureIdmagic8;
	GLuint gTextureIdMouse;
	GLuint gTextureIdScroll;
	
	bool gIsHatOn = true;

	Meshes meshes;
	//Shader Program
	GLuint gProgramId;
	//Camera
	Camera gCamera(glm::vec3(0.0f, 1.0f, 8.0f));

	glm::vec3 gCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 gCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 gCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 gCameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
}
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

bool perspective = false;

const GLchar* vertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	FragPos = vec3(model * vec4(aPos, 1.0));
	Normal = mat3(transpose(inverse(model))) * aNormal;
	TexCoords = aTexCoords;

	gl_Position = projection * view * vec4(FragPos, 1.0);
}
);

/* Fragment Shader Source Code */
const GLchar* fragmentShaderSource = GLSL(440,
	out vec4 FragColor;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};

struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float intensity;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float intensity;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[5];
uniform Material material;

uniform bool hasTexture;
uniform bool hasTextureTransparency;
uniform vec3 meshColor;

// function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
	// properties
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);


	vec3 result = CalcDirLight(dirLight, norm, viewDir);
	for (int i = 0; i < 5; i++)
	{
		result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
	}

	if (hasTextureTransparency)
		FragColor = vec4(result, texture(material.diffuse, TexCoords).a);
	else
		FragColor = vec4(result, 1.0);
}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	// combine results

	vec3 texColor;

	if (hasTexture)
		texColor = vec3(texture(material.diffuse, TexCoords));
	else
		texColor = meshColor;

	vec3 ambient = light.ambient * texColor;
	vec3 diffuse = light.diffuse * diff * texColor;
	vec3 specular = light.specular * spec * vec3(0.5, 0.5, 0.5);
	return (ambient + diffuse + specular) * light.intensity;
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	// diffuse shading
	float diff = max(dot(normal, lightDir), 0.0);
	// specular shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	// attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	vec3 texColor;

	if (hasTexture)
		texColor = vec3(texture(material.diffuse, TexCoords));
	else
		texColor = meshColor;

	// combine results
	vec3 ambient = light.ambient * texColor;
	vec3 diffuse = light.diffuse * diff * texColor;
	vec3 specular = light.specular * spec;
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	return (ambient + diffuse + specular) * light.intensity;
}
);




// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = 0; i < width * channels; ++i) // Fix the loop condition here
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}

int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	glfwSetCursorPosCallback(gWindow, UMousePositionCallback);

	// Set the mouse scroll callback
	glfwSetScrollCallback(gWindow, UMouseScrollCallback);
	glfwSetInputMode(gWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
	meshes.CreateMeshes();

	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
		return EXIT_FAILURE;
	// Load textures

	// Load textures
	if (!UCreateTexture("Textures/Keyboard.jpg", gTextureIdKeyboard)) {
		std::cerr << "Failed to load keyboard texture" << std::endl;
		// Handle error gracefully
	}
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);

	// Load and bind keys texture
	if (!UCreateTexture("Textures/roses.jpg", gTextureIdBoxes)) {
		std::cerr << "Failed to load keys texture" << std::endl;
		// Handle error gracefully
	}

	// Load and bind desk texture
	if (!UCreateTexture("Textures/desktop.jpg", gTextureIdDesk)) {
		std::cerr << "Failed to load desk texture" << std::endl;
		// Handle error gracefully
	}


	if (!UCreateTexture("Textures/rippednotes.jpg", gTextureIdNotebook)) {
		std::cerr << "Failed to load rippednotes texture" << std::endl;
		// Handle error gracefully
	}

	if (!UCreateTexture("Textures/iphone.png", gTextureIdPhone)) {
		std::cerr << "Failed to load phone texture" << std::endl;
		// Handle error gracefully
	}
	if (!UCreateTexture("Textures/magic8.png", gTextureIdmagic8)) {
		std::cerr << "Failed to load magic8 texture" << std::endl;
		// Handle error gracefully
	}
	
	if (!UCreateTexture("Textures/mouse.jpg", gTextureIdMouse)) {
		std::cerr << "Failed to mouse texture" << std::endl;
		// Handle error gracefully
	}
	
	if (!UCreateTexture("Textures/scroll.jpg", gTextureIdScroll)) {
		std::cerr << "Failed to load desk texture" << std::endl;
		// Handle error gracefully
	}

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);





	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing
	   // --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;
		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}
	
	meshes.DestroyMeshes();
	//destroying textures
	UDestroyTexture(gTextureIdKeyboard);
	UDestroyTexture(gTextureIdBoxes);
	UDestroyTexture(gTextureIdDesk);

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
	glfwInit();
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

	float cameraOffset = gCameraSpeed * gDeltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessInput(FORWARD, cameraOffset);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessInput(BACKWARD, cameraOffset);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessInput(LEFT, cameraOffset);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessInput(RIGHT, cameraOffset);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessInput(UP, cameraOffset * 0.5);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessInput(DOWN, cameraOffset * 0.5);



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
		gCameraSpeed *= 1.1f;
	else if (yoffset < 0.0)
		gCameraSpeed /= 1.1f;
}





void URender() {
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
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -1.0f, 100.0f);
		float cameraSpeed = 0.01f;
		if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
			gCameraPos.y += cameraSpeed;
		if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
			gCameraPos.y -= cameraSpeed;
		if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
			gCameraPos.x -= cameraSpeed;
		if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
			gCameraPos.x += cameraSpeed;
	}
	// Set the shader to be used
	glUseProgram(gProgramId);


	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");
	objectColorLoc = glGetUniformLocation(gProgramId, "uObjectColor");

	glUniform1i(glGetUniformLocation(gProgramId, "hasTextureTransparency"), 0);

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	
	glUniform3fv(glGetUniformLocation(gProgramId, "viewPos"), 1, glm::value_ptr(gCameraPos));

	glUniform1i(glGetUniformLocation(gProgramId, "material.diffuse"), 0);
	glUniform1f(glGetUniformLocation(gProgramId, "material.shininess"), 32.0f);


	// directional light

	glUniform3fv(glGetUniformLocation(gProgramId, "dirLight.direction"), 1, glm::value_ptr(glm::vec3(-0.2f, -1.0f, -0.3f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "dirLight.ambient"), 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.05f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "dirLight.diffuse"), 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.4f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "dirLight.specular"), 1, glm::value_ptr(glm::vec3(0.5f, 0.5f, 0.5f)));
	glUniform1f(glGetUniformLocation(gProgramId, "dirLight.intensity"), 2.0f);


	// point light 1
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[0].position"), 1, glm::value_ptr(glm::vec3(0.0f, 3.0f, 0.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[0].ambient"), 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.05f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[0].diffuse"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.8f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[0].specular"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.8f)));
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[0].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[0].linear"), 0.09);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[0].quadratic"), 0.032);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[0].intensity"), 2.5f);

	// point light 2
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[1].position"), 1, glm::value_ptr(glm::vec3(-7.0f, 3.0f, -8.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[1].ambient"), 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.05f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[1].diffuse"), 1, glm::value_ptr(glm::vec3(0.0f, 0.8f, 0.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[1].specular"), 1, glm::value_ptr(glm::vec3(0.0f, 0.8f, 0.0f)));
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[1].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[1].linear"), 0.09);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[1].quadratic"), 0.032);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[1].intensity"), 1.5f);

	// point light 3
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[2].position"), 1, glm::value_ptr(glm::vec3(8.0f, 3.0f, -8.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[2].ambient"), 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.05f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[2].diffuse"), 1, glm::value_ptr(glm::vec3(1.0f, 0.7f, 0.7f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[2].specular"), 1, glm::value_ptr(glm::vec3(1.0f, 0.7f, 0.7f)));
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[2].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[2].linear"), 0.09);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[2].quadratic"), 0.032);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[2].intensity"), 3.0f);

	// point light 4
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[3].position"), 1, glm::value_ptr(glm::vec3(-8.0f, 3.0f, 8.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[3].ambient"), 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.05f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[3].diffuse"), 1, glm::value_ptr(glm::vec3(0.0f, 0.8f, 0.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[3].specular"), 1, glm::value_ptr(glm::vec3(0.0f, 0.8f, 0.0f)));
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[3].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[3].linear"), 0.09);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[3].quadratic"), 0.032);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[3].intensity"), 1.0f);

	// point light 5
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[4].position"), 1, glm::value_ptr(glm::vec3(8.0f, 3.0f, 8.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[4].ambient"), 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.05f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[4].diffuse"), 1, glm::value_ptr(glm::vec3(0.8f, 0.0f, 0.0f)));
	glUniform3fv(glGetUniformLocation(gProgramId, "pointLights[4].specular"), 1, glm::value_ptr(glm::vec3(0.8f, 0.0f, 0.0f)));
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[4].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[4].linear"), 0.09);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[4].quadratic"), 0.032);
	glUniform1f(glGetUniformLocation(gProgramId, "pointLights[4].intensity"), 1.0f);

	glm::vec3 boardPosition(0.0f, 0.0f, -1.0f);

	//KEYBOARD BODY
	glBindVertexArray(meshes.gBoxMesh.vao);

	auto parentScale = glm::scale(glm::vec3(0.5f, 1.0f, 0.5f));

	scale = glm::scale(glm::vec3(8.0f, 0.1f, 3.0f));
	rotation = glm::rotate(glm::radians(15.0f), glm::vec3(1.0, 0.0f, 0.0f));
	translation = glm::translate(boardPosition);
	model = parentScale */*rotation**/translation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 0);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.8f, 0.8f, 0.8f)));
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);


	//KEYS
	glBindVertexArray(meshes.gBoxMesh.vao);
	// Define the number of keys in each row and column
	int numKeysX = 15; // Number of keys across
	int numKeysZ = 4; // Number of keys down

	float spacingX = 0.5f;
	float spacingZ = 0.5f;

	glm::vec3 startPos(-3.50f, 0.0f, 1.0f);

	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 0);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));

	for (int z = 0; z < numKeysZ; ++z)
	{
		for (int x = 0; x < numKeysX; ++x)
		{
			float posX = boardPosition.x + startPos.x + x * spacingX;
			float posY = boardPosition.y + startPos.y;
			float posZ = boardPosition.z + startPos.z - z * spacingZ;

			scale = glm::scale(glm::vec3(.25f, .25f, .25f));
			translation = glm::translate(glm::vec3(posX, posY, posZ));
			model = parentScale */*rotation**/translation * scale;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
		}
	}
	glBindVertexArray(0);

	//DESKTOP
	glBindVertexArray(meshes.gPlaneMesh.vao);
	scale = glm::scale(glm::vec3(6.0f, 6.0f, 6.0f));
	rotation = glm::rotate(45.0f, glm::vec3(1.0, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
	model = scale * translation;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glBindTexture(GL_TEXTURE_2D, gTextureIdDesk);
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 1);
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);


	//ripped notes
	glBindVertexArray(meshes.gPlaneMesh.vao);
	scale = glm::scale(glm::vec3(0.66f, 1.0f, 1.0f));
	rotation = glm::rotate(glm::radians(10.0f), glm::vec3(0.0, 1.0f, 0.0f));
	translation = glm::translate(glm::vec3(0.0f, 0.1f, 3.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glBindTexture(GL_TEXTURE_2D, gTextureIdNotebook); // Desk texture
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 1);
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);

	//magic 8 ball
	glBindVertexArray(meshes.gSphereMesh.vao);
	glm::mat4 sphereScale2 = glm::scale(glm::vec3(0.4f, 0.4f, 0.4f));
	glm::mat4 sphereTranslation2 = glm::translate(glm::vec3(-4.0f, 0.3f, 0.07f));
	glm::mat4 sphereModel2 = sphereTranslation2 * sphereScale2;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel2));
	glBindTexture(GL_TEXTURE_2D, gTextureIdmagic8);
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 1);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);

	//pencil
	glBindVertexArray(meshes.gSphereMesh.vao);
	glm::mat4 sphereScale = glm::scale(glm::vec3(0.04f, 0.04f, 0.04f));
	glm::mat4 sphereTranslation = glm::translate(glm::vec3(-1.52f, 0.099f, 2.07f));
	glm::mat4 sphereModel = sphereTranslation * sphereScale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel));
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 0);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.2f)));
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0); 
	glBindVertexArray(meshes.gCylinderMesh.vao);
	glm::mat4 cylinderScale = glm::scale(glm::vec3(0.03f, 1.5f, 0.03f));
	glm::mat4 cylinderRotation = glm::rotate(glm::radians(-70.0f), glm::vec3(0.0, 1.0f, 0.0f));
	cylinderRotation = glm::rotate(cylinderRotation, glm::radians(90.0f), glm::vec3(0.0, 0.0f, 1.0f));
	glm::mat4 cylinderTranslation = glm::translate(glm::vec3(-1.0f, 0.1f, 3.5f));
	glm::mat4 cylinderModel = cylinderTranslation * cylinderRotation * cylinderScale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cylinderModel));
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 0);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.8f, 0.8f, 0.8f)));
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);      // bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);     // top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);  // sides
	glBindVertexArray(0);

	glBindVertexArray(meshes.gConeMesh.vao);
	glm::mat4 coneScale = glm::scale(glm::vec3(0.06f, 0.04f, 0.04f));
	glm::mat4 coneTranslation = glm::translate(glm::vec3(0.8, 0.1f, 2.07f));
	glm::mat4 coneModel = coneTranslation * coneScale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(coneModel));
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 0);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.92f, 0.87f, 0.74f)));
	glDrawElements(GL_TRIANGLES, meshes.gConeMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);


	//iphone side
	glBindVertexArray(meshes.gBoxMesh.vao);
	scale = glm::scale(glm::vec3(0.48f, 0.05f, 1.0f));
	rotation = glm::rotate(glm::radians(190.0f), glm::vec3(0.0, 1.0f, 0.0f));
	translation = glm::translate(glm::vec3(-2.0f, 0.05f, 3.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 0);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.8f, 0.8f, 0.8f)));
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);

	//iphone 
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniform1i(glGetUniformLocation(gProgramId, "hasTextureTransparency"), 0);
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 0);
	glBindVertexArray(meshes.gPlaneMesh.vao);
	scale = glm::scale(glm::vec3(0.24f, 0.1f, 0.5f));
	rotation = glm::rotate(glm::radians(190.0f), glm::vec3(0.0, 1.0f, 0.0f));
	translation = glm::translate(glm::vec3(-2.0f, 0.08f, 3.0f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glBindTexture(GL_TEXTURE_2D, gTextureIdPhone);
	glUniform1i(glGetUniformLocation(gProgramId, "hasTextureTransparency"), 1);
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 1);
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
	glDisable(GL_BLEND);

	//mouse
	glBindVertexArray(meshes.gSphereMesh.vao);
	glm::mat4 sphereScale3 = glm::scale(glm::vec3(0.3f, 0.08f, 0.5f));
	glm::mat4 sphereTranslation3 = glm::translate(glm::vec3(1.3f, 0.1f, 2.5f));
	glm::mat4 sphereModel3 = sphereTranslation3 * sphereScale3;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel3));
	glBindTexture(GL_TEXTURE_2D, gTextureIdMouse);
	glUniform1i(glGetUniformLocation(gProgramId, "hasTexture"), 1);
	glUniform3fv(glGetUniformLocation(gProgramId, "meshColor"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);


	
	glBindVertexArray(meshes.gTorusMesh.vao);
	scale = glm::scale(glm::vec3(0.06f, 0.07f, 0.2f));
	rotation = glm::rotate(glm::radians(120.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	translation = glm::translate(glm::vec3(1.3f, 0.13f, 2.5f));
	model = translation * rotation * scale;
	glBindTexture(GL_TEXTURE_2D, gTextureIdScroll);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glProgramUniform4f(gProgramId, objectColorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);
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
		glBindTexture(GL_TEXTURE_2D, 1);
		glBindTexture(GL_TEXTURE_2D, 2);

		return true;
	}

	// Error loading the image
	return false;
}
void UDestroyTexture(GLuint textureId)
{
	glDeleteTextures(1, &textureId);
}