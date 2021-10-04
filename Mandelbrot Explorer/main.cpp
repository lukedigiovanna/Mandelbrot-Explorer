
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
#include <locale>
#include <codecvt>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

double x = 0.0, y = 0.0;
double scale = 1.0;
bool mouseDown = false;
double lastX, lastY;
bool mouseCalled = false;
int width, height;
double mx = 0.0, my = 0.0;
int maxIterations = 64;

bool zooming = false;
double zoomLocation[] = { 0, 0 };

unsigned int zoomIndex = 0;

void saveImage(const char* filepath, GLFWwindow* w) {
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (!zooming) {
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
	if (!zooming) {
		if (yoffset == 1) // then we assume the mouse is noncontinuous, so set the offset to be some constant
			yoffset = -0.3;
		else if (yoffset == -1)
			yoffset = 0.3;

		double off = 1 + yoffset;
		if (off > 0) {
			zoom(mx / width * scale * 2 + (x - scale), (height - my) / height * scale * 2 + (y - scale), off);
		}
	}
}

void clear_console(HANDLE console) {
	DWORD written = 0;
	for (SHORT i = 0; i < 120; i++)
		for (SHORT j = 0; j < 30; j++)
			WriteConsoleOutputCharacter(console, L" ", 1, { i, j }, &written);
}

std::wstring to_wstring_p(const double val, const int n = 6)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << val;
	std::string s = out.str();
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(s);
	return wide;

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

		if (!zooming) {
			WriteConsoleOutputCharacter(console, L"MANDELBROT EXPLORER", 19, { 2, 1 }, &written);
			std::wstring fields[6 ] = {
				L"MAX_ITERATIONS:  " + std::to_wstring(maxIterations),
				L"RENDER_TIME: " + std::to_wstring(elapsed),
				L"FPS: " + to_wstring_p(rollingFPSSum / fpsBuffer.size(), 2),
				L"REAL: " + to_wstring_p(mx / width * 2 * scale + (x - scale), 20),
				L"IMAGINARY: " + to_wstring_p((height - my) / height * 2 * scale + (y - scale), 20),
				L"SCALE: " + to_wstring_p(scale, 20)
			};
			for (int i = 0; i < 6; i++) {
				WriteConsoleOutputCharacter(console, fields[i].c_str(), fields[i].length(), { (SHORT)3, (SHORT)3 + (SHORT)i }, &written);
			}

			std::wstring controls[] = {
				L"LEFT CLICK + DRAG: PAN",
				L"SCROLL: ZOOM",
				L"UP KEY: INCREASE ITERATIONS",
				L"DOWN KEY: INCREASE ITERATIONS",
				L"R: BEGIN A RENDERED ZOOM",
				L"ESC: STOP ZOOM"
			};
			WriteConsoleOutputCharacter(console, L"CONTROLS", 8, { 2, 10 }, &written);
			for (int i = 0; i < 6; i++) {
				WriteConsoleOutputCharacter(console, controls[i].c_str(), controls[i].length(), { (SHORT)3, (SHORT)12 + (SHORT)i }, &written);
			}

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			{
				maxIterations++;
			}

			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
				maxIterations--;
			}

			if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
				zooming = true;

				clear_console(console);
				
				std::cout << "What does this do?" << std::endl;
				std::cout << "   This automatically zooms in on a partciular location on the mandelbrot set." << std::endl;
				std::cout << "   An MP4 of the rendered zoom will be output in the directory of this executable." << std::endl << std::endl;
				std::cout << "--RENDERED ZOOM INSTRUCTIONS--" << std::endl;
				std::cout << std::endl;
				std::cout << "ZOOM LOCATION:" << std::endl;
				// get zoom location
				std::cout << "  REAL: ";
				std::cin >> zoomLocation[0];
				std::cout << "  IMAGINARY: ";
				std::cin >> zoomLocation[1];
				if (zoomLocation[0] == 0 && zoomLocation[1] == 0) {
					zoomLocation[0] = mx / width * scale * 2 + (x - scale);
					zoomLocation[1] = (height - my) / height * scale * 2 + (y - scale);
				}
				scale = 1.0;
				x = zoomLocation[0];
				y = zoomLocation[1];
				zoomIndex = 0;

				clear_console(console);
			}
		}
		else {
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || scale < 0.0000000000000002) {
				zooming = false;
				clear_console(console);
			}


			zoom(zoomLocation[0], zoomLocation[1], 0.99);
			
			saveImage(("render/" + std::to_string(zoomIndex) + ".png").c_str(), window);
			zoomIndex++;
			
			WriteConsoleOutputCharacter(console, L"MANDELBROT EXPLORER", 19, { 2, 1 }, &written);
			std::wstring fields[6] = {
				L"MAX_ITERATIONS:  " + std::to_wstring(maxIterations),
				L"RENDER_TIME: " + std::to_wstring(elapsed),
				L"FPS: " + to_wstring_p(rollingFPSSum / fpsBuffer.size(), 2),
				L"REAL: " + to_wstring_p(zoomLocation[0], 20),
				L"IMAGINARY: " + to_wstring_p(zoomLocation[1], 20),
				L"SCALE: " + to_wstring_p(scale, 20)
			};
			for (int i = 0; i < 6; i++) {
				WriteConsoleOutputCharacter(console, fields[i].c_str(), fields[i].length(), { (SHORT)3, (SHORT)3 + (SHORT)i }, &written);
			}
		}
	}

	glfwTerminate();

	return 0;
}
	