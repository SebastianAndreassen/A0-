#include <stdio.h>  	// fprintf, stdout, stderr.
#include <stdlib.h> 	// exit, EXIT_FAILURE, EXIT_SUCCESS.
#include <string.h> 	// strerror.
#include <errno.h>  	// errno.
#include <unistd.h> 	// file checks
#include <stdbool.h> 	// booleans

// Check if the file encoding is ASCII
static bool is_ascii(unsigned char *buffer, size_t file_length) {
	for (size_t i = 0; i < file_length; i++) {
		unsigned char byte = buffer[i];
		if (!(byte < 0x80) || byte == 0x00) { 			// if byte value is larger than 127/or NUL -> not ASCII
			return false;
		}
	}
	return true;
}

// Check if the file encoding is ISO-8859-1
static bool is_iso(unsigned char *buffer, size_t file_length) {
	bool has_iso = false;
	for (size_t i = 0; i < file_length; i++) {
		unsigned char byte = buffer[i];
		if (byte == 0x00) { 						// if byte value is NUL -> not ISO
			return false;
		} else if (byte >= 0x80) {					// if byte value is equal or greater than 128 -> has ISO byte
			has_iso = true;
		} else {
			continue;
		}
	}
	return has_iso;
}

// Check if the file encoding is UTF-8
static bool is_utf(unsigned char *buffer, size_t file_length) {
	size_t i = 0;
	while (i < file_length) {
		unsigned char byte = buffer[i];
		if (byte == 0x00) { 						// if byte is NUL -> data
			return false;
		}
		if ((byte & 0x80) == 0x00) {				// 0xxxxxxx (1 byte)
			i += 1;
		} else if ((byte & 0xE0) == 0xC0) {			// 110xxxxx 10xxxxxx (2 bytes)
			if (i + 1 >= file_length) {
				return false;
			} 
			if ((buffer[i + 1] & 0xC0) != 0x80) {
				return false;
			}
			i += 2;
		} else if ((byte & 0xF0) == 0xE0) {			// 1110xxxx 10xxxxxx 10xxxxxx (3 bytes)
			if (i + 2 >= file_length) {
				return false;
			} 
			if ((buffer[i + 1] & 0xC0) != 0x80) {
				return false;
			}
			if ((buffer[i + 2] & 0xC0) != 0x80) {
				return false;
			}
			i += 3;
		} else if ((byte & 0xF8) == 0xF0) {			// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (4 bytes)
			if (i + 3 >= file_length) {
				return false;
			}
			if ((buffer[i + 1] & 0xC0) != 0x80) {
				return false;
			}
			if ((buffer[i + 2] & 0xC0) != 0x80) {
				return false;
			}
			if ((buffer[i + 3] & 0xC0) != 0x80) {
				return false;
			}
			i += 4;
		} else {
			return false;
		}
	}
	return true;
} 

int print_error(const char *file_name, int errnum) {
	return fprintf(stdout, "%s: cannot determine (%s)\n", file_name, strerror(errnum));
} 

void detect_file_type(const char *file_name) {
	// Check if file exists and if it is readible 
    if (access(file_name, F_OK) != 0 || access(file_name, R_OK) != 0) {
		print_error(file_name, errno);
        return;
    }
	// Try opening the file
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        fprintf(stderr, "%s: error when opening file: %s\n", file_name, strerror(errno));
        return;
    }
	// Find the length of the file
	fseek(file, 0, SEEK_END);
	if (fseek(file, 0, SEEK_END) != 0) { 
		fprintf(stderr, "%s: cannot seek end of file: %s\n", file_name, strerror(errno)); 
		fclose(file); 
		return; 
	} 
	long file_length = ftell(file);
	if (file_length < 0) { 
		fprintf(stderr, "%s: cannot tell size of file: %s\n", file_name, strerror(errno)); 
		fclose(file); 
		return; 
	}
	if (file_length == 0) {
		printf("%s: empty\n", file_name);
		fclose(file);
		return;
	}
	if ((unsigned long)file_length > __LONG_MAX__) {
		fprintf(stderr, "%s: file too large: %s\n", file_name, strerror(errno));
		fclose(file);
		return;
	}
	rewind(file);
	// Memory allocate the buffer in the size of the file length
	unsigned char *buffer = malloc(file_length);
	if (!buffer) {
		fprintf(stderr, "Could not allocate memory for buffer: %s", strerror(errno));
		fclose(file);
		return;
	}
	// Read the file, and write to the buffer
	fread(buffer, 1, file_length, file);
	fclose(file);
	// Check for file encoding
	if (is_ascii(buffer, file_length)) {
		printf("%s: ASCII text\n", file_name);
		return;
	} else if (is_utf(buffer, file_length)) {
		printf("%s: UTF-8 Unicode text\n", file_name);
		return;
	} else if (is_iso(buffer, file_length)) {
		printf("%s: ISO-8859 text\n", file_name);
	} else {
		printf("%s: data\n", file_name);
		return;
	} 
	// Clear the memory allocated to the buffer
	free(buffer);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
    	printf("Use: %s <file_name>\n", argv[0]);
    	return EXIT_FAILURE;
	}

	detect_file_type(argv[1]);
	return EXIT_SUCCESS;
}