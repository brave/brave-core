/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

Vector LinearAlgebraUtil::AddVectorScalar(Vector vector, float scalar) {
  Eigen::Map<Eigen::VectorXf> vector_eigen(vector.data(), vector.size());

  auto array = vector_eigen.array();
  array += scalar;
  vector_eigen = array.matrix();
  return MatrixXfToVector(vector_eigen);
}

Vector LinearAlgebraUtil::AddVectors(Vector vector_1, Vector vector_2) {
  Eigen::Map<Eigen::VectorXf> vector_1_eigen(vector_1.data(), vector_1.size());
  Eigen::Map<Eigen::VectorXf> vector_2_eigen(vector_2.data(), vector_2.size());

  vector_1_eigen += vector_2_eigen;
  return MatrixXfToVector(vector_1_eigen);
}

Vector LinearAlgebraUtil::SubtractVector(Vector vector_1, Vector vector_2) {
  Eigen::Map<Eigen::VectorXf> vector_1_eigen(vector_1.data(), vector_1.size());
  Eigen::Map<Eigen::VectorXf> vector_2_eigen(vector_2.data(), vector_2.size());
  vector_1_eigen -= vector_2_eigen;

  return MatrixXfToVector(vector_1_eigen);
}

Vector LinearAlgebraUtil::MultiplyVectorScalar(Vector vector, float scalar) {
  Eigen::Map<Eigen::VectorXf> vector_eigen(vector.data(), vector.size());

  auto array = vector_eigen.array();
  array *= scalar;
  vector_eigen = array.matrix();
  return MatrixXfToVector(vector_eigen);
}

Vector LinearAlgebraUtil::MultiplyMatrixVector(Matrix matrix, Vector vector) {
  Eigen::MatrixXf matrix_eigen = MatrixToMatrixXf(matrix);
  Eigen::Map<Eigen::VectorXf> vector_eigen(vector.data(), vector.size());

  Eigen::MatrixXf result = matrix_eigen * vector_eigen;
  return MatrixXfToVector(result);
}

Matrix LinearAlgebraUtil::TransposeMatrix(Matrix matrix) {
  Eigen::MatrixXf matrix_eigen = MatrixToMatrixXf(matrix);

  matrix_eigen.transposeInPlace();
  return MatrixXfToMatrix(matrix_eigen);
}

Vector LinearAlgebraUtil::MatrixXfToVector(const Eigen::MatrixXf& vector) {
  Vector returned_vector = Vector(vector.rows());
  Eigen::VectorXf::Map(returned_vector.data(), vector.rows()) = vector;

  return returned_vector;
}

Matrix LinearAlgebraUtil::MatrixXfToMatrix(const Eigen::MatrixXf& matrix) {
  Matrix returned_matrix = Matrix(matrix.rows(), Vector(matrix.cols(), 0.0f));
  for (size_t i = 0; i < returned_matrix.size(); ++i) {
    Eigen::VectorXf::Map(returned_matrix.at(i).data(), matrix.row(i).cols()) =
        matrix.row(i);
  }

  return returned_matrix;
}

Eigen::MatrixXf LinearAlgebraUtil::MatrixToMatrixXf(Matrix matrix) {
  if (matrix.empty()) {
    return Eigen::MatrixXf();
  }

  Eigen::MatrixXf matrix_eigen(matrix.size(), matrix.at(0).size());
  for (size_t i = 0; i < matrix.size(); ++i) {
    matrix_eigen.row(i) =
        Eigen::Map<Eigen::VectorXf>(matrix.at(i).data(), matrix.at(i).size());
  }

  return matrix_eigen;
}

}  // namespace brave_federated
