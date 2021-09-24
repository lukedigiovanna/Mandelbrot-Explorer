#version 400 core

out vec4 fragColor;
in vec4 vertexColor;

uniform dvec2 resolution;

uniform dvec2 centerPosition;
uniform double scale;

float norm(float _x) {
	//return _x * 2.0 - 1.0;
	return _x;
}

uniform double test;

void main() {
	dvec2 coord = gl_FragCoord.xy/resolution * 2.0 - dvec2(1.0, 1.0);
	 
	dvec2 c = coord * scale + centerPosition; // transform the coord
	dvec2 z = c;

	float iterations = 1.0;
	float maxIterations = 256.0;
	while (iterations < maxIterations && z.x * z.x + z.y * z.y < 4.0) {
		z = dvec2(z.x * z.x - z.y * z.y + c.x, 2.0 * z.x * z.y + c.y);
		iterations++;
	}

	iterations /= 10.0;

	//float n = float(length(z));
	float n = iterations;

	fragColor = vec4(norm(sin(n + 4.51)), norm(sin(n + 2.45)), norm(sin(n + 5.45)), 1.0);
};