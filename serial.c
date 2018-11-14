#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define filterWidth 3
#define filterHeight 3

#define RGB_COMPONENT_COLOR 255

typedef struct {
	 unsigned char r, g, b;
} PPMPixel;

void writeImage(PPMPixel *image, char *name, unsigned long int width, unsigned long int height)
{
	FILE *fp;
	fp = fopen(name, "wb");
	if (!fp) {
		fprintf(stderr, "Unable to open file '%s'\n", name);
		exit(1);
	}

	fprintf(fp, "%s\n%lu %lu\n%d\n", "P6", width, height, RGB_COMPONENT_COLOR);
	if (fwrite(image, 3 * width, height, fp) != height) {		 
		fprintf(stderr, "Error writing image 'out.ppm'\n");
		exit(1);
	}

	fclose(fp);
}

PPMPixel *readImage(const char *filename, unsigned long int *width, unsigned long int *height)
{
	char buff[16];
	PPMPixel *img;
	FILE *fp;
	int c, rgb_comp_color;
   
	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//read image format
	if (!fgets(buff, sizeof(buff), fp)) {
		perror(filename);
		exit(1);
	}

	//check the image format
	if (buff[0] != 'P' || buff[1] != '6') {
		 fprintf(stderr, "Invalid image format (must be 'P6')\n");
		 exit(1);
	}

	//check for comments
	c = getc(fp);
	while (c == '#') {
		while (getc(fp) != '\n');
		c = getc(fp);
	}
	ungetc(c, fp);
	
	//read image size information
	if (fscanf(fp, "%lu %lu", width, height) != 2) {
		 fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
		 exit(1);
	}

	//alloc memory form image
	img = (PPMPixel *)malloc(*width * *height * sizeof(PPMPixel));
	if (!img) {
		 fprintf(stderr, "Unable to allocate memory\n");
		 exit(1);
	}

	//read rgb component
	if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
		 fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
		 exit(1);
	}

	//check rgb component depth
	if (rgb_comp_color!= RGB_COMPONENT_COLOR) {
		 fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
		 exit(1);
	}

	while (fgetc(fp) != '\n');
	//read pixel data from file
	if (fread(img, 3 * *width, *height, fp) != *height) {
		 fprintf(stderr, "Error loading image '%s'\n", filename);
		 exit(1);
	}

	fclose(fp);
	return img;
}

PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime, int debug) {
	
	unsigned long int x, y, filterX, filterY;
	double red, green, blue;
	struct timeval start, end;
	double laplacian_factor = 1.0;
	double laplacian[filterHeight][filterWidth] =
	{
	  -1, -1, -1,
	  -1,  8, -1,
	  -1, -1, -1,
	};

	PPMPixel *result = (PPMPixel*) malloc(w * h * sizeof(PPMPixel));
	if (!result) {
		 fprintf(stderr, "Unable to allocate memory\n");
		 exit(1);
	}

	gettimeofday(&start, NULL);
	//apply the filter
	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			red = 0.0, green = 0.0, blue = 0.0;

			//multiply every value of the filter with corresponding image pixel
			for(filterY = 0; filterY < filterHeight; filterY++)
				for(filterX = 0; filterX < filterWidth; filterX++) {

				int imageX = (x - filterWidth / 2 + filterX + w) % w;
				int imageY = (y - filterHeight / 2 + filterY + h) % h;
				red += image[imageY * w + imageX].r * laplacian[filterY][filterX];
				green += image[imageY * w + imageX].g * laplacian[filterY][filterX];
				blue += image[imageY * w + imageX].b * laplacian[filterY][filterX];
			}
			
			//truncate values smaller than zero and larger than 255
			red = laplacian_factor * red;
			if ((int)(red) < 0) {
				result[y * w + x].r = 0;
			} else if ((int)(red) > 255) {
				result[y * w + x].r = 255;
			} else {
				result[y * w + x].r = red;
			}
			green = laplacian_factor * green;
			if ((int)(green) < 0) {
				result[y * w + x].g = 0;
			} else if ((int)(green) > 255) {
				result[y * w + x].g = 255;
			} else {
				result[y * w + x].g = green;
			}
			blue = laplacian_factor * blue;
			if ((int)(blue) < 0) {
				result[y * w + x].b = 0;
			} else if ((int)(blue) > 255) {
				result[y * w + x].b = 255;
			} else {
				result[y * w + x].b = blue;
			}
		}
	}

	gettimeofday(&end, NULL);

	*elapsedTime = end.tv_sec - start.tv_sec;
	*elapsedTime += (end.tv_usec - start.tv_usec) / 1000000.0;

	return result;
}

int main(int argc, char *argv[])
{
	//load the image into the buffer
	unsigned long int w, h, debug = 0;
	double elapsedTime = 0.0;

	if (argc < 2) {
		fprintf(stderr, "No image given as parameter\n");
		exit(1);
	}

	if (argc == 3) debug = 1;

	PPMPixel *image = readImage(argv[1], &w, &h);
	
	PPMPixel *result = apply_filters(image, w, h, &elapsedTime, debug);
	
	printf("Time consumed: %.3f s\n", elapsedTime);
	
	if (debug == 1) {
		writeImage(result, "laplacian.ppm", w, h);
	}
	
	free(result);
	free(image);
	return 0;
}
