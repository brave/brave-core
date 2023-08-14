/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_LINEAR_ALGEBRA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_LINEAR_ALGEBRA_UTIL_H_

#include <Eigen/Dense>
#include <vector>

namespace brave_federated {

// aliases
using Weights = std::vector<float>;
using Sample = std::vector<float>;
using Vector = std::vector<float>;
using DataSet = std::vector<Sample>;
using Matrix = std::vector<Vector>;

namespace linear_algebra_util {

Vector AddVectorScalar(Vector vector, float scalar);
Vector AddVectors(Vector vector_1, Vector& vector_2);
Vector SubtractVector(Vector vector_1, Vector& vector_2);

Vector MultiplyVectorScalar(Vector vector, float scalar);
Vector MultiplyMatrixVector(Matrix matrix, Vector& vector);

Matrix TransposeMatrix(Matrix& matrix);

Vector MatrixXfToVector(const Eigen::MatrixXf& vector);
Matrix MatrixXfToMatrix(const Eigen::MatrixXf& matrix);
Eigen::MatrixXf MatrixToMatrixXf(Matrix& matrix);

}  // namespace linear_algebra_util
}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_LINEAR_ALGEBRA_UTIL_H_
