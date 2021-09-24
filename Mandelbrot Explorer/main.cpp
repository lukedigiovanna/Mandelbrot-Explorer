
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

static double x = 0.0, y = 0.0;
static double scale = 1.0;
static bool mouseDown = false;
static double lastX, lastY;
static bool mouseCalled = false;
static int width, height;
static double mx = 0.0, my = 0.0;

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (!mouseCalled) {
		lastX = xpos;
		lastY = ypos;
		mouseCalled = true;
	}
	else {

		double dx = xpos - lastX;
		double dy = ypos - lastY;
		lastX = xpos;
		lastY = ypos;

		if (mouseDown) {
			x -= dx / width * scale * 2;
			y += dy / height * scale * 2;
		}
	}
}

// zooms in the fractal view centered on a specific x, y position in fractal space
// more accurately, centered on a particular complex number (real component is x, imaginary is y)
static void zoom(double xlocation, double ylocation, double scaleFactor) {
	if (scaleFactor > 0) {
		double prevScale = scale;
		scale *= scaleFactor;
		x = scale + xlocation - scale / prevScale * (xlocation - x + prevScale);
		y = scale + ylocation - scale / prevScale * (ylocation - y + prevScale);
	}
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	if (yoffset == 1) // then we assume the mouse is noncontinuous, so set the offset to be some constant
		yoffset = -0.3;
	else if (yoffset == -1)
		yoffset = 0.3;

	double off = 1 + yoffset;
	if (off > 0) {
		zoom(mx / width * scale * 2 + (x - scale), (height - my) / height * scale * 2 + (y - scale), off);
	}
}

int main() {
	if (!glfwInit())
		return -1; // error!

	GLFWwindow* window = glfwCreateWindow(640, 480, "Mandelbrot Set", NULL, NULL);
	if (!window) {
		return -1; // error!
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
		return -1;

	glfwSwapInterval(1);

	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// SET UP SHADERS
	unsigned int vsId = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fsId = glCreateShader(GL_FRAGMENT_SHADER);

	std::ifstream vsFile, fsFile;
	std::string vsCode, fsCode;
	vsFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fsFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		vsFile.open("vertexShader.glsl");
		fsFile.open("fragmentShader.glsl");
		std::stringstream vsStream;
		std::stringstream fsStream;
		vsStream << vsFile.rdbuf();
		fsStream << fsFile.rdbuf();
		vsFile.close();
		fsFile.close();
		vsCode = vsStream.str();
		fsCode = fsStream.str();
	}
	catch (std::ifstream::failure e) {
		std::cout << "Error reading shader files" << std::endl;
	}

	const char* vsCodePointer = vsCode.c_str();
	glShaderSource(vsId, 1, &vsCodePointer, NULL);
	const char* fsCodePointer = fsCode.c_str();
	glShaderSource(fsId, 1, &fsCodePointer, NULL);

	glCompileShader(vsId);
	// get any error messages
	int success;
	char infoLog[512];
	unsigned int shaders[2] = { vsId, fsId };
	for (unsigned int shader : shaders) {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vsId, 512, NULL, infoLog);
			std::cout << infoLog << std::endl;
		}
	}

	glCompileShader(fsId);

	unsigned int program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, fsId);
	glLinkProgram(program);

	glDetachShader(program, vsId);
	glDetachShader(program, fsId);

	glDeleteShader(vsId);
	glDeleteShader(fsId);

	glUseProgram(program);
	unsigned int vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	float vertices[18] = {
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		-1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		GLuint resLocation = glGetUniformLocation(program, "resolution");
		glUniform2d(resLocation, width, height);
		GLuint centerLocation = glGetUniformLocation(program, "centerPosition");
		glUniform2d(centerLocation, x, y);
		GLuint sizeLocation = glGetUniformLocation(program, "scale");
		glUniform1d(sizeLocation, scale);

		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		glfwGetCursorPos(window, &mx, &my);

		mouseDown = state == GLFW_PRESS;

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
	