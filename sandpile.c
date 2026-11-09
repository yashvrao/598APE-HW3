#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void die(const char *s) {
  fprintf(stderr, "%s\n", s);
  exit(1);
}

int N = 801;
int **grid;
int *row_start;
int *row_end;

// you can also use this function to debug print your sandpile
static void render(char *filename) {
  int chan = 3;
  unsigned char *pixels = (unsigned char *)malloc(N * N * chan * sizeof(char));
  int stride = N * chan;
  if (!pixels)
    die("malloc failed for PNG pixel buffer");
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      unsigned char r = 0, g = 0, b = 0;
      switch (grid[i][j]) {
      case 0: // obsidian
        r = 15;
        g = 15;
        b = 25;
        break;
      case 1: // electric cyan
        r = 0;
        g = 210;
        b = 255;
        break;
      case 2: // vibrant magenta
        r = 230;
        g = 30;
        b = 130;
        break;
      case 3: // bright gold
        r = 255;
        g = 200;
        b = 0;
        break;
      default: // white
        r = 0;
        g = 0;
        b = 0;
      }

      int pid = (i * N + j) * chan;
      pixels[pid] = r;
      pixels[pid + 1] = g;
      pixels[pid + 2] = b;
    }
  }
  if (stbi_write_png(filename, N, N, chan, pixels, stride)) {
    printf("Successfully saved sandpile identity to: %s\n", filename);
  } else {
    die("Error: stb_image_write failed to save png file\n");
  }
  free(pixels);
}

static void stabilize_full() {
  int changed = 0;
  do {
    changed = 0;
    for (int i = 0; i < N; ++i) {
      for (int j = row_start[i]; j <= row_end[i]; ++j) {
        if (grid[i][j] < 4)
          continue;
        changed = 1; // topple it
        int q = grid[i][j] / 4;
        grid[i][j] -= 4 * q;
        if (i > 0)
          grid[i - 1][j] += q;
        if (j > 0)
          grid[i][j - 1] += q;
        if (i < N - 1)
          grid[i + 1][j] += q;
        if (j < N - 1)
          grid[i][j + 1] += q;
      }
    }
  } while (changed);
}


static void stabilize() {
  if (N % 2 == 0) {
    stabilize_full();
    return;
  }

  int center = N / 2;
  int changed;
  do {
    changed = 0;
    for (int i = 0; i <= center; ++i) {
      for (int j = row_start[i]; j <= row_end[i]; ++j) {
        if (grid[i][j] < 4)
          continue;
        changed = 1;
        int q = grid[i][j] / 4;
        grid[i][j] -= 4 * q;
        if (i > 0)
          grid[i - 1][j] += q;
        if (i < center)
          grid[i + 1][j] += (i + 1 == center ? 2 : 1) * q;
        if (j > 0)
          grid[i][j - 1] += q;
        if (j < N - 1)
          grid[i][j + 1] += q;
      }
    }
  } while (changed);

  /* Restore every stored cell, including inactive sinks, before the next phase. */
  for (int i = 0; i < center; ++i)
    memcpy(grid[N - 1 - i], grid[i], (size_t)N * sizeof(*grid[i]));
}

#ifdef HW3_GRID_DUMP
static void dump_grid(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) die("failed to open grid dump");
  for (int i = 0; i < N; ++i)
    if (fwrite(grid[i], sizeof(*grid[i]), (size_t)N, f) != (size_t)N)
      die("failed to write grid dump");
  if (fclose(f)) die("failed to close grid dump");
}
#endif

void find_identity() {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6;
    }
  }
  stabilize();
#ifdef HW3_GRID_DUMP
  dump_grid("phase1-grid.bin");
#endif
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6 - grid[i][j];
    }
  }
  stabilize();
#ifdef HW3_GRID_DUMP
  dump_grid("phase2-grid.bin");
#endif
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 2) {
    fprintf(stderr, "Usage: %s N\n", argv[0]);
    return EXIT_FAILURE;
  }
  char *end;
  errno = 0;
  uintmax_t parsed = strtoumax(argv[1], &end, 10);
  if (errno || argv[1][0] < '0' || argv[1][0] > '9' || *end || !parsed ||
      parsed > INT32_MAX)
    die("grid size must be a positive int");
  N = (int)parsed;

  grid = (int **)malloc(N * sizeof(int *));
  for (int i = 0; i < N; ++i) {
    grid[i] = (int *)malloc(N * sizeof(int));
  }

  row_start = (int *)malloc(N * sizeof(int));
  row_end = (int *)malloc(N * sizeof(int));
  if (!row_start || !row_end)
    die("malloc failed for row bounds");
  for (int i = 0; i < N; ++i) {
    row_start[i] = N;
    row_end[i] = -1;
    for (int j = 0; j <= MAX(i, N - 1 - i); ++j) {
      if ((i - N / 2) * (i - N / 2) + (j - N / 2) * (j - N / 2) <=
          (N / 2) * (N / 2)) {
        if (row_start[i] == N)
          row_start[i] = j;
        row_end[i] = j;
      }
    }
  }

  find_identity();

  render("identity.png");
  for (int i = 0; i < N; ++i) {
    free(grid[i]);
  }
  free(grid);
  free(row_start);
  free(row_end);
  return EXIT_SUCCESS;
}
