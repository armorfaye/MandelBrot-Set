#include <vector>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <emscripten/bind.h>

const int MAX_ITER = 1000;
const int H = 500; 

struct Color{
    int r, g, b; 	
}; 

std::vector<Color> palette(H); 

void generateColorPalette() {
    const double frequency = 2 * M_PI / H;
    const double amplitude = 127; // Max value for color component / 2
    const double center = 128;    // Max value for color component / 2

    for (int i = 0; i < H; ++i) {
        palette[i].g = static_cast<int>(std::sin(frequency * i + 0) * amplitude + center);
        palette[i].r = static_cast<int>(std::sin(frequency * i + 2 * M_PI / 3) * amplitude + center);
        palette[i].b = static_cast<int>(std::sin(frequency * i + 4 * M_PI / 3) * amplitude + center);
    }
}

int getIterations(double real, double imag) {
    int iteration = 0;
    double x = 0;
    double y = 0;
    double x2 = 0;
    double y2 = 0;
    double lastX = 0;
    double lastY = 0; 
    int period = 0; 

    while(x2 + y2 <= 4 && iteration < MAX_ITER) {
        y = 2 * x * y + imag;
        x = x2 - y2 + real;
        x2 = x * x;
        y2 = y * y;

        if(x == lastX && y == lastY){
            return MAX_ITER;
        }

        while(++period > 9){
            lastX = x;
            lastY = y;
            period = 0; 
        }

        iteration++;
    }

    if (iteration < MAX_ITER) {
        for (int extra = 0; extra < 2; ++extra) {
            y = 2 * x * y + imag;
            x = x2 - y2 + real;
            x2 = x * x;
            y2 = y * y;
            iteration++;
        }

        double modulus = sqrt(x2 + y2);
        double mu = iteration - log(log(modulus)) / log(2.0);
        iteration = static_cast<int>(mu);
    }

    return iteration;
}

void computeMandelbrot(int WIDTH, int HEIGHT, std::vector<std::vector<int> > &results, double start_real, double start_imag, double end_real, double end_imag) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0 ; y < HEIGHT; y++) {
            double real = start_real + x * ((end_real - start_real) / WIDTH);
            double imag = end_imag - y * ((end_imag - start_imag) / HEIGHT);
            results[x][y] = getIterations(real, imag);
        }
    }
}

std::vector<Color> genPixels(int WIDTH, double start_real, double start_imag, double end_real, double end_imag) {
	std::cout << start_real << " " << start_imag << " " << end_real << " " << end_imag << "\n";
	std::vector<Color> pixels; 

    double height = (WIDTH * (end_imag - start_imag)) / (end_real - start_real);

    int HEIGHT = static_cast<int>(height);

    std::vector<std::vector<int> > results(WIDTH, std::vector<int>(HEIGHT, 0));

    generateColorPalette(); 

    computeMandelbrot(WIDTH, HEIGHT, results, start_real, start_imag, end_real, end_imag);

	for(int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int iterations = results[x][y];

			if(iterations == MAX_ITER){
				Color color = { 0, 0, 0 }; 
				pixels.push_back(color);
				continue; 
			}
            float hue = (float)((iterations) * H / MAX_ITER);
            int index = int(hue) % H;
			Color color = {palette[index].r, palette[index].g, palette[index].b};
			pixels.push_back(color);
        }
    }
    return pixels;
}

using namespace emscripten;
EMSCRIPTEN_BINDINGS(my_module) {
	value_object<Color>("Color")
		.field("r", &Color::r)
		.field("g", &Color::g)
		.field("b", &Color::b);
    function("genPixels", &genPixels);
	register_vector<Color>("ColorVector");
}
