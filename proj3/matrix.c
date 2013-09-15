/* Project 3: Matrix Multiplication Project (matrix.c)
 * Corey Johns
 * COP4610 Spring 2013
 * Deadline: 3/24/12
 *
 * Perform matrix multiplication project from textbook using pthreads.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define M 3
#define K 2
#define N 3

int A [M][K] = { {1,4}, {2,5}, {3,6} };
int B [K][N] = { {8,7,6}, {5,4,3} };
int C [M][N];

struct v {
  int i; // Row
  int j; // Column
};

void *multiply(void *arg){
  struct v* param = (struct v *)arg;
  int i = param->i;
  int j = param->j;
  C[i][j] = 0;

  int k;
  for (k = 0; k < K; k++)
    C[i][j] += A[i][k] * B[k][j];
    
  free(arg); // Free data that was malloc'd earlier?
  pthread_exit(NULL);
  return 0;
}

// Passing multidimensional arrays of arbitrary depth is hard
/*void print_matrix(int ** matrix, int rows, int cols){
  int i, j;
  for(i = 0; i < rows; i++){
    for (j = 0; j < cols; j++){
      if (j != cols-1)
        printf("%d ", matrix[i][j]);
      else
        printf("%d", matrix[i][j]);
    }
    printf("\n");
  }
}*/

int main(){
  // Perform multiplication
  int i, j, rc;
  pthread_t threads[M*N];
  for (i = 0; i < M; i++){
    for (j = 0; j < N; j++){
      struct v *data = (struct v *) malloc(sizeof(struct v));
      data->i = i;
      data->j = j;
      rc = pthread_create(&threads[i*N + j], NULL, multiply, (void *)data);
      if (rc){
        printf("* ERROR: pthread_create() abnormal return value.\n");
        exit(1);
      }
    }
  }

  for (i = 0; i < M*N; i++)
    pthread_join(threads[i], NULL); // Wait for the previous threads to finish

  // Print the matrices
  printf("Matrix A:\n");
  for(i = 0; i < M; i++){
    for (j = 0; j < K; j++){
      if (j != K-1)
        printf("%d ", A[i][j]);
      else
        printf("%d", A[i][j]);
    }
    printf("\n");
  }

  printf("\nMatrix B:\n");
  for(i = 0; i < K; i++){
    for (j = 0; j < N; j++){
      if (j != N-1)
        printf("%d ", B[i][j]);
      else
        printf("%d", B[i][j]);
    }
    printf("\n");
  }

  printf("\nMatrix C:\n");
  for(i = 0; i < M; i++){
    for (j = 0; j < N; j++){
      if (j != N-1)
        printf("%d ", C[i][j]);
      else
        printf("%d", C[i][j]);
    }
    printf("\n");
  }

  pthread_exit(NULL);
  return 0;
}
