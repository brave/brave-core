/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_LINEAR_ALGEBRA_UTIL_LINEAR_ALGEBRA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_LINEAR_ALGEBRA_UTIL_LINEAR_ALGEBRA_UTIL_H_

#include <vector>

namespace brave_federated {

// aliases
using Weights = std::vector<float>;
using Sample = std::vector<float>;
using Vector = std::vector<float>;
using DataSet = std::vector<Sample>;
using Matrix = std::vector<Vector>;

class LinearAlgebraUtil {
 public:
  static Vector SubtractVector(Vector v1, Vector v2);

  static Vector MultiplyMatrixVector(Matrix mat, Vector v);

  static Matrix MultiplyMatrices(Matrix mat1, Matrix mat2);

  static Vector AddVectorScalar(Vector v, float a);

  static Vector AddVectors(Vector v1, Vector v2);

  static Vector MultiplyVectorScalar(Vector v, float a);

  static Matrix TransposeVector(Matrix v);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_UTIL_LINEAR_ALGEBRA_UTIL_LINEAR_ALGEBRA_UTIL_H_
