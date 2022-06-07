#include "brave/components/brave_federated/linear_algebra_util/linear_algebra_util.h"

#include <vector>

namespace brave_federated {

std::vector<float> LinearAlgebraUtil::SubtractVector(std::vector<float> v1,
                                                     std::vector<float> v2) {
  std::vector<float> result(v1.size());
  for (int i = 0; i < (int)v1.size(); i++) {
    result[i] = v1[i] - v2[i];
  }

  return result;
}

std::vector<float> LinearAlgebraUtil::MultiplyMatrixVector(
    std::vector<std::vector<float>> mat,
    std::vector<float> v) {
  std::vector<float> result(mat.size(), 0.0);
  for (int i = 0; i < (int)mat.size(); i++) {
    result[i] = 0;
    for (int j = 0; j < (int)mat[0].size(); j++) {
      result[i] += mat[i][j] * v[j];
    }
  }

  return result;
}

std::vector<float> LinearAlgebraUtil::AddVectorScalar(std::vector<float> v,
                                                      float a) {
  for (int i = 0; i < (int)v.size(); i++) {
    v[i] += a;
  }

  return v;
}

std::vector<float> LinearAlgebraUtil::AddVectors(std::vector<float> v1,
                                                      std::vector<float> v2) {
  for (int i = 0; i < (int)v1.size(); i++) {
    v1[i] += v2[i];
  }

  return v1;
}

std::vector<float> LinearAlgebraUtil::MultiplyVectorScalar(std::vector<float> v,
                                                           float a) {
  for (int i = 0; i < (int)v.size(); i++) {
    v[i] *= a;
  }

  return v;
}

std::vector<std::vector<float>> LinearAlgebraUtil::MultiplyMatrices(
    std::vector<std::vector<float>> mat1,
    std::vector<std::vector<float>> mat2) {
      std::vector<std::vector<float>> result(mat1.size(), std::vector<float>(mat2[0].size(), 0.0));

  for (int i = 0; i < (int) mat1.size(); i++) {
    for (int j = 0; j < (int) mat2[0].size(); j++) {
      for (int k = 0; k < (int) mat1[0].size(); k++) {
        result[i][j] += mat1[i][k] * mat2[k][j];
      }
    }
  }
  
  return result;
}

std::vector<std::vector<float>> LinearAlgebraUtil::TransposeVector(
    std::vector<std::vector<float>> v) {
  std::vector<std::vector<float>> vT(v[0].size(), std::vector<float>(v.size()));
  for (int i = 0; i < (int)v.size(); i++) {
    for (int j = 0; j < (int)v[0].size(); j++) {
      vT[j][i] = v[i][j];
    }
  }

  return vT;
}
}  // namespace brave_federated