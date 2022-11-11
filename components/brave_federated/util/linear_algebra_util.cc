/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/util/linear_algebra_util.h"

#include <vector>

namespace brave_federated {

Vector LinearAlgebraUtil::SubtractVector(Vector v1, Vector v2) {
  Vector result(v1.size());
  for (size_t i = 0; i < v1.size(); i++) {
    result[i] = v1[i] - v2[i];
  }

  return result;
}

Vector LinearAlgebraUtil::MultiplyMatrixVector(Matrix mat, Vector v) {
  Vector result(mat.size(), 0.0);
  for (size_t i = 0; i < mat.size(); i++) {
    result[i] = 0;
    for (size_t j = 0; j < mat[0].size(); j++) {
      result[i] += mat[i][j] * v[j];
    }
  }

  return result;
}

Vector LinearAlgebraUtil::AddVectorScalar(Vector v, float a) {
  for (size_t i = 0; i < v.size(); i++) {
    v[i] += a;
  }

  return v;
}

Vector LinearAlgebraUtil::AddVectors(Vector v1, Vector v2) {
  for (size_t i = 0; i < v1.size(); i++) {
    v1[i] += v2[i];
  }

  return v1;
}

Vector LinearAlgebraUtil::MultiplyVectorScalar(Vector v, float a) {
  for (size_t i = 0; i < v.size(); i++) {
    v[i] *= a;
  }

  return v;
}

Matrix LinearAlgebraUtil::MultiplyMatrices(Matrix mat1, Matrix mat2) {
  Matrix result(mat1.size(), Vector(mat2[0].size(), 0.0));

  for (size_t i = 0; i < mat1.size(); i++) {
    for (size_t j = 0; j < mat2[0].size(); j++) {
      for (size_t k = 0; k < mat1[0].size(); k++) {
        result[i][j] += mat1[i][k] * mat2[k][j];
      }
    }
  }

  return result;
}

Matrix LinearAlgebraUtil::TransposeVector(Matrix v) {
  Matrix vT(v[0].size(), Vector(v.size()));
  for (size_t i = 0; i < v.size(); i++) {
    for (size_t j = 0; j < v[0].size(); j++) {
      vT[j][i] = v[i][j];
    }
  }

  return vT;
}

}  // namespace brave_federated
