/*

Heat 2D distribution
A rectangular plate of material has heat applied to the center of its upper
edge. The temperature itself is fixed at the boundary: the edges of the plate
are held at constant temperatures (the upper edge is held at high temp, other
edges are held at low temp). The plate is modeled as a grid of points - size of
grid is N. The Laplace equation dictates how the heat will transfer from grid
point to grid point over time until the steady-state is achieved (the
temperature will not change any more.) The program simulates the diffusion of
temperature in all points over time.

This program assumes that N is divisible by number of processes.

Compile with:
gcc -g mpi_heat.c -I "c:\Program Files (x86)\Microsoft SDKs\MPI\Include" -L
"c:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -lmsmpi -o mpi_heat.exe

*/
#include "mpi.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define MASTER 0

#define N 1000 /* grid points */
#define MAXITER 2000
#define EPSILON 1e-6
// #define DEBUG

double start_time_p;

void Allocate_and_init(int rank, double **grid, double **new_grid,
                       int local_rows);
void Exchange_frontiers(int rank, int size, double *grid, int local_rows);
void Compute_local(int rank, int size, double *grid, double *new_grid,
                   int local_rows);
void Aggregate_final(int rank, double *grid, int local_rows);

int is_converged(int rank, int size, double *grid, double *new_grid,
                 int local_rows);

int main(int argc, char *argv[]) {
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (N % size != 0) {
    if (rank == 0)
      printf("N=%d is not divisible by size=%d \n", N, size);
    MPI_Finalize();
    exit(1);
  }

  double *grid, *new_grid;   // local grid chunks
  int local_rows = N / size; // rows per rank
  int status;

  // Allocate chunks and scatter initial data
  Allocate_and_init(rank, &grid, &new_grid, local_rows);

  while (!is_converged(rank, size, grid, new_grid, local_rows)) {
    Exchange_frontiers(rank, size, grid, local_rows);

    Compute_local(rank, size, grid, new_grid, local_rows);

    double *tmp = grid;
    grid = new_grid;
    new_grid = tmp;
  }

  // Gather chunks into final result
  Aggregate_final(rank, grid, local_rows);

  free(grid);
  free(new_grid);

  MPI_Finalize();
}

int is_converged(int rank, int size, double *grid, double *new_grid,
                 int local_rows) {
  int converged = 1;
  for (int i = 1; i < local_rows + 1; i++) {
    for (int j = 1; j < N + 1; j++) {
      if (fabs(grid[i * N + j] - new_grid[i * N + j]) > EPSILON) {
        converged = 0;
        break;
      }
    }
  }
  return converged;
}

/*  Init temperature in all grid points.
    Hardcoded - apply heat on upper edge
    */
void init_temperatures(double *grid, int size) {
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      grid[i * size + j] = 20;
    }
  }
  /* init boundary conditions:
  apply heat on middle part of upper boundary */
  for (int j = size / 3; j < size * 2 / 3; j++) {
    grid[0 * size + j] = 100;
  }
}

/* print grid values */
void print_grid(double *grid, int rows, int cols) {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      printf("%4.2lf ", grid[i * cols + j]);
    }
    printf("\n");
  }
  fflush(stdout);
}

/* Every process will get its own chunks of the grid.
   The chunks contain also 2 additional rows(the halo).
   The chunks will be initialized with data - use  MPI_Scatter */
void Allocate_and_init(int rank, double **grid, double **new_grid,
                       int local_rows) {
  double *whole_grid = NULL;

  if (rank == 0) // only the master process allocates and initializes big grid
  {
    printf("Grid size N=%d\n", N);
    fflush(stdout);
    whole_grid = (double *)malloc(sizeof(double) * N * N);
    if (whole_grid == NULL) {
      printf("Rank %d: Cannot allocate whole grid\n", rank);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    init_temperatures(whole_grid, N);
#ifdef DEBUG
    printf("Initial grid: \n");
    print_grid(grid, N, N);
    fflush(stdout);
#endif
  }

  // every process, including master, allocates local grid chunks

  // local chunk includes 2 extra halo rows
  int local_N = local_rows + 2;

  *grid = (double *)malloc(sizeof(double) * local_N * N);
  *new_grid = (double *)malloc(sizeof(double) * local_N * N);
  if ((*grid == NULL) || (*new_grid == NULL)) {
    printf("Rank %d: Cannot allocate local grid\n", rank);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  if (rank == 0) {
    printf("Start parallel for N=%d and MAXITER=%d  ... \n", N, MAXITER);
    fflush(stdout);
    start_time_p = MPI_Wtime();
  }

  // Scatter chunks of initial matrix to workers
  MPI_Scatter(whole_grid, N * local_rows, MPI_DOUBLE, (*grid) + N,
              N * local_rows, MPI_DOUBLE, 0, MPI_COMM_WORLD);

#ifdef DEBUG
  printf("Initial chunk of process rank %d \n", rank);
  print_grid(grid, N, N);
  fflush(stdout);
#endif

  // initial big grid is  not  preserved
  if (rank == 0)
    free(whole_grid); // big matrix not needed any more
}

/* Performs the frontier exchanges of a process*/
void Exchange_frontiers(int rank, int size, double *grid, int local_rows) {
  MPI_Status status;

  int top_real = 1 * N;
  int top_halo = 0 * N;
  int bottom_real = local_rows * N;
  int bottom_halo = (local_rows + 1) * N;

  // Send real top row UP / receive from neighbor into top halo
  if (rank > 0) {
    MPI_Sendrecv(grid + top_real, N, MPI_DOUBLE, rank - 1, 0, grid + top_halo,
                 N, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &status);
  }

  // Send real bottom row DOWN / receive from neighbor into bottom halo
  if (rank < size - 1) {
    MPI_Sendrecv(grid + bottom_real, N, MPI_DOUBLE, rank + 1, 0,
                 grid + bottom_halo, N, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD,
                 &status);
  }
}

/* performs update of points from own chunk */
void Compute_local(int rank, int size, double *grid, double *new_grid,
                   int local_rows) {
  // loop own rows (1 .. local_rows) â€” these are the rows we update
  for (int i = 1; i <= local_rows; ++i) {
    // preserve left & right boundary columns (they are fixed)
    new_grid[i * N + 0] = grid[i * N + 0];
    new_grid[i * N + (N - 1)] = grid[i * N + (N - 1)];

    // update interior columns j=1..N-2 using 5-point stencil neighbors
    for (int j = 1; j <= N - 2; ++j) {
      new_grid[i * N + j] =
          0.25 * (grid[(i - 1) * N + j] + // top (may be halo)
                  grid[(i + 1) * N + j] + // bottom (may be halo)
                  grid[i * N + (j - 1)] + // left
                  grid[i * N + (j + 1)]); // right
    }
  }

  // preserve global top boundary row (row 1) if this is the first process
  if (rank == 0) {
    for (int j = 0; j < N; ++j)
      new_grid[1 * N + j] = grid[1 * N + j];
  }

  // preserve global bottom boundary row (row local_rows) if this is the last
  // process
  if (rank == size - 1) {
    for (int j = 0; j < N; ++j)
      new_grid[(local_rows)*N + j] = grid[(local_rows)*N + j];
  }
}

/* Assembles the fina grid - uses MPI_Gather to collect chunks from all
 * processes */
void Aggregate_final(int rank, double *grid, int local_rows) {
  double *whole_grid = (double *)malloc(sizeof(double) * N * N);
  if (whole_grid == NULL) {
    printf("Rank %d: Cannot allocate final big grid\n", rank);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  MPI_Gather(grid + N, N * local_rows, MPI_DOUBLE, whole_grid, N * local_rows,
             MPI_DOUBLE, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    double end_time_p = MPI_Wtime();
    printf("Parallel elapsed time: %f seconds\n", end_time_p - start_time_p);
#ifdef DEBUG
    printf("Final grid: \n");
    print_grid(whole_grid, N, N);
#endif
  }
}
