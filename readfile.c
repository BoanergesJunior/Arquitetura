
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FILE_SIZE 65536
#define LINE_SIZE 4

unsigned int memory[MAX_FILE_SIZE / 4];
unsigned short fileSize;
 	
// read binary file into memory
void loadBinary(const char *filename) {
    //Open the file for reading in binary mode
    FILE *fIn = fopen(filename, "rb");
    long position;
	
    if (fIn == NULL) {
	  printf("File not found: %s\n", filename);
	  exit(1);
	} else {
      //Go to the end of the file
      const int fseek_end_value = fseek(fIn, 0, SEEK_END);
      if (fseek_end_value != -1) {

        //Get the current position in the file (in bytes)
        position = ftell(fIn);
        if (position != -1) {

          //Go back to the beginning of the file
          const int fseek_set_value = fseek(fIn, 0, SEEK_SET);

		  //If error, exit
		  if (fseek_set_value == -1) {
			  printf("Error reading file.");
			  exit(1);
		  }

		  //If file too big, exit
		  if (fseek_set_value > MAX_FILE_SIZE) {
			  printf("Maximum allowed file size is 64KB.");
			  exit(1);
		  }

          //Read the whole file to memory
          fileSize = fread(memory, 1, position, fIn);
        }
      }
      fclose(fIn);
    }
}


void writeFileAsText (const char *basename) {
	char filename[256] = "txt_";
	FILE * fp;
	int i,j;

	strcat(filename,basename);
	strcat(filename,".txt");

	fp = fopen (filename, "w+");

	//legenda
	fprintf(fp, "ADDR    ");
	for (j=0; j<LINE_SIZE; j++) {
		fprintf(fp, "ADDR+%02X  ", 4*j);
	}
	fprintf(fp, "\n----------------------------------------------------------------------------\n");


	//programa
	i=0;
	for (i = 0; i < fileSize / 4; i+=LINE_SIZE) {
		fprintf(fp, "%04X    ", 4*i);
		for (j=0; j<LINE_SIZE; j++) {
			fprintf(fp, "%08X ", memory[i+j]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

int main(int argc, char *argv[]) {

    char *filename;
	
	printf("Usage: readfile [filename=summation.o].\n");
	if (argc < 2) {
		filename = "summation.o";
	} else {
		filename = argv[1];
	}
	printf("Transforming file %s.\n", filename);

	loadBinary(filename);
	writeFileAsText(filename);

	return 0;
}
