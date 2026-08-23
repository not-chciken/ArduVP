#define MATRIX_SIZE 8
#define NUM_ITERATIONS 50

int a[MATRIX_SIZE][MATRIX_SIZE];
int b[MATRIX_SIZE][MATRIX_SIZE];
int c[MATRIX_SIZE][MATRIX_SIZE];

void fill_random(int m[MATRIX_SIZE][MATRIX_SIZE]) {
  for (int i = 0; i < MATRIX_SIZE; i++) {
    for (int j = 0; j < MATRIX_SIZE; j++) {
      m[i][j] = random(0, 10);
    }
  }
}

void multiply(int a[MATRIX_SIZE][MATRIX_SIZE], int b[MATRIX_SIZE][MATRIX_SIZE],
              int c[MATRIX_SIZE][MATRIX_SIZE]) {
  for (int i = 0; i < MATRIX_SIZE; i++) {
    for (int j = 0; j < MATRIX_SIZE; j++) {
      int sum = 0;
      for (int k = 0; k < MATRIX_SIZE; k++) {
        sum += a[i][k] * b[k][j];
      }
      c[i][j] = sum;
    }
  }
}

void setup() {
  Serial.begin(9600);
  randomSeed(42);
  fill_random(a);
  fill_random(b);

  unsigned long start_time = millis();
  for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
    multiply(a, b, c);
  }
  unsigned long end_time = millis();
  unsigned long elapsed_time = end_time - start_time;

  Serial.print(NUM_ITERATIONS);
  Serial.print(" x ");
  Serial.print(MATRIX_SIZE);
  Serial.print("x");
  Serial.print(MATRIX_SIZE);
  Serial.print(" matrix multiplication took ");
  Serial.print(elapsed_time);
  Serial.println(" milliseconds");
}

void loop() {
}
