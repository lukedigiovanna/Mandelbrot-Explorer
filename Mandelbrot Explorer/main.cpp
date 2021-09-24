
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

#include <Windows.h>
#include <chrono>
#include <vector>

double x = 0.0, y = 0.0;
double scale = 1.0;
bool mouseDown = false;
double lastX, lastY;
bool mouseCalled = false;
int width, height;
double mx = 0.0, my = 0.0;
int maxIterations = 64;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
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
void zoom(double xlocation, double ylocation, double scaleFactor) {
	if (scaleFactor > 0) {
		double prevScale = scale;
		scale *= scaleFactor;
		x = scale + xlocation - scale / prevScale * (xlocation - x + prevScale);
		y = scale + ylocation - scale / prevScale * (ylocation - y + prevScale);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
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

	COORD topLeft = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	std::vector<double> fpsBuffer;
	double rollingFPSSum = 0;

	auto last = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		// this will run our shader, so begin timing here
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		GLuint resLocation = glGetUniformLocation(program, "resolution");
		glUniform2d(resLocation, width, height);
		GLuint centerLocation = glGetUniformLocation(program, "centerPosition");
		glUniform2d(centerLocation, x, y);
		GLuint sizeLocation = glGetUniformLocation(program, "scale");
		glUniform1d(sizeLocation, scale);
		glUniform1i(glGetUniformLocation(program, "maxIterations"), maxIterations);

		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		glfwGetCursorPos(window, &mx, &my);

		mouseDown = state == GLFW_PRESS;

		glfwSwapBuffers(window);
		glfwPollEvents();

		auto now = std::chrono::high_resolution_clock::now();
		double elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last).count() / 1000000000.0;
		last = now;
		double fps = 1 / elapsed;

		fpsBuffer.push_back(fps);
		rollingFPSSum += fps;
		if (fpsBuffer.size() > 100) {
			rollingFPSSum -= fpsBuffer.at(0);
			fpsBuffer.erase(fpsBuffer.begin());
		}

		WriteConsoleOutputCharacter(console, L"MANDELBROT EXPLORER", 19, { 2, 1 }, &written);
		std::wstring fields[5] = {
			L"MAX_ITERATIONS:  " + std::to_wstring(maxIterations),
			L"RENDER_TIME: " + std::to_wstring(elapsed),
			L"FPS: " + std::to_wstring(rollingFPSSum / fpsBuffer.size()),
			L"REAL: " + std::to_wstring(mx / width * 2 * scale + (x - scale)),
			L"IMAGINARY: " + std::to_wstring((height - my) / height * 2 * scale + (y - scale))
		};
		for (int i = 0; i < 5; i++) {
			WriteConsoleOutputCharacter(console, fields[i].c_str(), fields[i].length(), { 3, 3 + (SHORT)i }, &written);
		}

		int shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
		static bool shiftDown = false;
		if (shift == GLFW_PRESS)
			shiftDown = true;
		else if (shift == GLFW_RELEASE)
			shiftDown = false;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			if (shiftDown)
				maxIterations++;
			else
				maxIterations += 32;
		}
	}

	glfwTerminate();

	return 0;
}
	