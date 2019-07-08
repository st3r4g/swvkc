#ifndef MYALGEBRA_H
#define MYALGEBRA_H

void algebra_matrix_rotation_x(float *matrix, float theta);
void algebra_matrix_rotation_y(float *matrix, float theta);
void algebra_matrix_traslation(float *matrix, float x, float y, float z);
void algebra_matrix_multiply(float *product, float *a, float *b);
void algebra_matrix_ortho(float *matrix, float left, float right, float bottom,
float top, float near, float far);
void algebra_matrix_persp(float *m, float fov, float ratio, float n, float f);

#endif
