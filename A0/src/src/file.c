#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Helper from assignment
int print_error(char *path, int errnum) {
    return fprintf(stdout, "%s: cannot determine (%s)\n", path, strerror(errnum));
}

// ASCII allowed?
int is_ascii(unsigned char b) {
    if (b >= 0x07 && b <= 0x0D) return 1;
    if (b == 0x1B) return 1;
    if (b >= 0x20 && b <= 0x7E) return 1;
    return 0;
}

int looks_like_utf8(FILE *fp) {
    int c, saw_multibyte = 0;
    while ((c = fgetc(fp)) != EOF) {
        unsigned char b = (unsigned char)c;
        if (b <= 0x7F) continue;               // ASCII
        else if ((b & 0xE0) == 0xC0) {         // 2-byte
            if (b < 0xC2) return 0;            // reject 0xC0/0xC1 (overlong)
            int c1 = fgetc(fp);
            if (c1 == EOF || ((unsigned char)c1 & 0xC0) != 0x80) return 0;
            saw_multibyte = 1;
        } else if ((b & 0xF0) == 0xE0) {       // 3-byte
            int c1 = fgetc(fp), c2 = fgetc(fp);
            if (c2 == EOF) return 0;
            if (((unsigned char)c1 & 0xC0) != 0x80) return 0;
            if (((unsigned char)c2 & 0xC0) != 0x80) return 0;
            saw_multibyte = 1;
        } else if ((b & 0xF8) == 0xF0) {       // 4-byte
            int c1 = fgetc(fp), c2 = fgetc(fp), c3 = fgetc(fp);
            if (c3 == EOF) return 0;
            if (((unsigned char)c1 & 0xC0) != 0x80) return 0;
            if (((unsigned char)c2 & 0xC0) != 0x80) return 0;
            if (((unsigned char)c3 & 0xC0) != 0x80) return 0;
            saw_multibyte = 1;
        } else {
            return 0;                           // invalid lead
        }
    }
    return saw_multibyte;                       // ASCII-only ≠ “UTF-8 text”
}


// ISO-8859-1 allowed?
int is_iso(unsigned char b) {
    if (is_ascii(b)) return 1;
    if (b >= 0xA0) return 1;   // <=0xFF is redundant for unsigned char
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: file path\n");
        return EXIT_FAILURE;
    }

    char *path = argv[1];
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        print_error(path, errno);
        return EXIT_SUCCESS;
    }

    int c = fgetc(fp);
    if (c == EOF) {
        printf("%s: empty\n", path);
        fclose(fp);
        return EXIT_SUCCESS;
    }

    // First pass: ASCII/ISO flags
    int all_ascii = 1, all_iso = 1;
    do {
        unsigned char b = (unsigned char)c;
        if (!is_ascii(b)) all_ascii = 0;
        if (!is_iso(b))   all_iso   = 0;
    } while ((c = fgetc(fp)) != EOF);

    if (all_ascii) {
        printf("%s: ASCII text\n", path);
        fclose(fp);
        return EXIT_SUCCESS;
    }

    // Not ASCII — do a second pass for UTF-8
    rewind(fp);
    if (looks_like_utf8(fp)) {
        printf("%s: UTF-8 Unicode text\n", path);
        fclose(fp);
        return EXIT_SUCCESS;
    }

    // ISO or data
    if (all_iso) {
        printf("%s: ISO-8859 text\n", path);
    } else {
        printf("%s: data\n", path);
    }

    fclose(fp);
    return EXIT_SUCCESS;
}
