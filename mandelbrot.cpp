#include <vector>
#include <cmath>
#include <iostream>
#include <thread>
#include <png.h>
#include <cstdlib>
#include <cstdio>
#include <queue>

const int WIDTH = 4096;
int HEIGHT;
const int MAX_ITER = 500;
const int H = 500; 

const int DIR[4][2] = {{0,1}, {1,0},{-1,0},{0,-1}}; 

struct Color{
	uint8_t r, g, b; 
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
            int iterations = results[x][y] >> 1; 

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


void computeMandelbrot(std::vector<std::vector<int> > &results, const double &start_real, const double &start_imag, const double &end_real, const double &end_imag, int y_start, int y_end) {

	std::queue<std::pair<int, int>> bfs; 

	for(int i=0; i<WIDTH; i++){
		bfs.push(std::make_pair(i, y_start));
		bfs.push(std::make_pair(i, y_end-1));
		results[i][y_start] = 1;
		results[i][y_end - 1] = 1;
	}

	for(int i=y_start+1; i<y_end-1; i++){
		bfs.push(std::make_pair(0, i));
		bfs.push(std::make_pair(WIDTH-1, i));
		results[0][i] = 1;
		results[WIDTH-1][i] = 1;
	}

	while(!bfs.empty()){
		auto cur = bfs.front(); 
		bfs.pop(); 

		int cx = cur.first;
		int cy = cur.second; 

		double real = start_real + cx * ((end_real - start_real) / WIDTH);
		double imag = end_imag - cy * ((end_imag - start_imag) / HEIGHT);

		int iters = getIterations(real, imag);

		results[cx][cy] = (iters << 1); 

		if(iters == MAX_ITER){
			continue; 
		}

		for(const int *vec : DIR){
			int nx = cx + vec[0];
			int ny = cy + vec[1];

			if(nx < 0 || nx >= WIDTH || ny < y_start || ny >= y_end){
				continue; 
			}

			if(results[nx][ny] != 0){
				continue; 
			}

			results[nx][ny] = 1;
			bfs.push(std::make_pair(nx, ny));
		}
	}

	for(int i = 0; i < WIDTH; i++){
		for(int j = y_start; j<y_end; j++){
			if((results[i][j] >> 1) == 0){
				results[i][j] = (MAX_ITER << 1) + 1; 
			}
		}
	}
}

int main(int argc, char *argv[]) {
	double start_real = atof(argv[1]);
	double start_imag = atof(argv[2]);
	double end_real = atof(argv[3]);
	double end_imag = atof(argv[4]);

	double height = (WIDTH*(end_imag - start_imag))/(end_real - start_real);

	HEIGHT = (int)height; 

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
