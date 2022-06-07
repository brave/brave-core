#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LINEAR_ALGEBRA_UTIL_LINEAR_ALGEBRA_UTIL_H
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LINEAR_ALGEBRA_UTIL_LINEAR_ALGEBRA_UTIL_H

#include <vector>

namespace brave_federated {

class LinearAlgebraUtil {
 public:
  static std::vector<float> SubtractVector(std::vector<float> v1,
                                           std::vector<float> v2);

  static std::vector<float> MultiplyMatrixVector(
      std::vector<std::vector<float>> mat,
      std::vector<float> v);

  static std::vector<std::vector<float>> MultiplyMatrices(
      std::vector<std::vector<float>> mat1,
      std::vector<std::vector<float>> mat2);

  static std::vector<float> AddVectorScalar(std::vector<float> v, float a);

  static std::vector<float> AddVectors(std::vector<float> v1, std::vector<float> v2);

  static std::vector<float> MultiplyVectorScalar(std::vector<float> v, float a);

  static std::vector<std::vector<float>> TransposeVector(
      std::vector<std::vector<float>> v);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LINEAR_ALGEBRA_UTIL_LINEAR_ALGEBRA_UTIL_H
