#include <gmpxx.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <thread>
#include <png.h>
#include <cstdlib>
#include <cstdio>
#include <emscripten/bind.h>

const int WIDTH = 4096;
int HEIGHT;
const int MAX_ITER = 500;
const int H = 500; 


struct Color{
	uint8_t r, g, b; 
}; 

std::vector<Color> palette(H); 

void generateColorPalette() {
    const double frequency = 2 * M_PI / H;
    const double amplitude = 127; // Max value for color component / 2
    const double center = 128;    // Max value for color component / 2

    for (int i = 0; i < H; ++i) {
        palette[i].r = static_cast<uint8_t>(std::sin(frequency * i + 0) * amplitude + center);
        palette[i].g = static_cast<uint8_t>(std::sin(frequency * i + 4 * M_PI / 3) * amplitude + center);
        palette[i].b = static_cast<uint8_t>(std::sin(frequency * i + 2 * M_PI / 3) * amplitude + center);
    }
}

int getIterations(const mpf_class &real, const mpf_class &imag) {
    int iteration = 0;
    mpf_class x = 0;
    mpf_class y = 0;
    mpf_class x2 = 0;
    mpf_class y2 = 0;
	mpf_class lastX = 0;
	mpf_class lastY = 0; 
	//mpf_class x1 = 0, y1 = 0; // Variables for Brent's algorithm
    //int lam = 0;
    //int power = 1;
    //nt steps_taken = 0;

	int period = 0; 

    while(x2 + y2 <= 4 && iteration < MAX_ITER) {
        y = 2 * x * y + imag;
        x = x2 - y2 + real;
        x2 = x * x;
        y2 = y * y;

        /*
		if (iteration == power) {
            x1 = x;
            y1 = y;
            power *= 2;
            lam = 0;
            steps_taken = 0;
        }

		if ((x == x1 && y == y1) && steps_taken > 0) {
            // A cycle is detected, we can assume the point is in the Mandelbrot set
            return MAX_ITER;
        }
        steps_taken++;
        lam++; */

		if(x == lastX && y ==lastY){
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

        mpf_class modulus = sqrt(x2 + y2);
        mpf_class mu = iteration - log(log(modulus.get_d())) / log(2.0);
        iteration = static_cast<int>(mu.get_d());
    }

    return iteration;
}

// Function to create a PNG image
void create_png(const char *filename, const int &width, const int &height, std::vector<std::vector<int> > &results) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    // Allocate memory for one row (3 bytes per pixel - RGB)
    png_bytep row = (png_bytep) malloc(3 * width * sizeof(png_byte));

    // Write image data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int iterations = results[x][y];

			if(iterations == MAX_ITER){
				row[x*3] = 0;    // Red value
            	row[x*3 + 1] = 0;  // Green value
            	row[x*3 + 2] = 0; //Blue value
				continue; 
			}
            float hue = static_cast<float>(iterations) * H / MAX_ITER;
            int index = static_cast<int>(hue) % H;
            row[x*3] = palette[index].r;
            row[x*3 + 1] = palette[index].g;
            row[x*3 + 2] = palette[index].b;
        }
        png_write_row(png, row);
    }

    // End write
    png_write_end(png, NULL);

    fclose(fp);

    if (png && info)
        png_destroy_write_struct(&png, &info);
    if (row)
        free(row);
}


void computeMandelbrot(std::vector<std::vector<int> > &results, const mpf_class &start_real, const mpf_class &start_imag, const mpf_class &end_real, const mpf_class &end_imag, int y_start, int y_end) {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = y_start; y < y_end; y++) {
            mpf_class real = start_real + x * ((end_real - start_real) / WIDTH);
            mpf_class imag = end_imag - y * ((end_imag - start_imag) / HEIGHT);
            results[x][y] = getIterations(real, imag);
        }
    }
}


int main(int argc, char *argv[]) {
	mpf_class start_real(argv[1]);
	mpf_class start_imag(argv[2]);
	mpf_class end_real(argv[3]);
	mpf_class end_imag(argv[4]);

	mpf_class height = (WIDTH*(end_imag - start_imag))/(end_real - start_real);

	HEIGHT = (int)height.get_d();

	std::vector<std::vector<int> > results(WIDTH, std::vector<int>(HEIGHT, 0));

	generateColorPalette(); 

	unsigned num_threads = 8;
    std::vector<std::thread> threads(num_threads);
   

	int block_size = HEIGHT / num_threads;
    for (unsigned i = 0; i < num_threads; ++i) {
        int y_start = i * block_size;
        int y_end = (i == num_threads - 1) ? HEIGHT : y_start + block_size;
        threads[i] = std::thread(computeMandelbrot, std::ref(results), std::ref(start_real), std::ref(start_imag), std::ref(end_real), std::ref(end_imag), y_start, y_end);
    }

    // Join the threads with the main thread
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    create_png("mandelbrot.png", WIDTH, HEIGHT, results);

    return 0;
}
