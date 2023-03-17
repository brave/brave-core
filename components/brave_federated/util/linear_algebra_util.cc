/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/util/linear_algebra_util.h"

namespace brave_federated {

Vector LinearAlgebraUtil::MatrixXfToVector(Eigen::MatrixXf v) {
  Vector returned_v = Vector(v.rows());
  Eigen::VectorXf::Map(returned_v.data(), v.rows()) = v;

  return returned_v;
}

Matrix LinearAlgebraUtil::MatrixXfToMatrix(Eigen::MatrixXf mat) {
  Matrix returned_mat = Matrix(mat.rows(), Vector(mat.cols(), 0.0f));
  for (size_t i = 0; i < returned_mat.size(); ++i) {
    Eigen::VectorXf::Map(returned_mat.at(i).data(), mat.row(i).cols()) =
        mat.row(i);
  }

  return returned_mat;
}

Eigen::MatrixXf LinearAlgebraUtil::MatrixToMatrixXf(Matrix mat) {
  Eigen::MatrixXf mat_(mat.size(), mat.at(0).size());
  for (size_t i = 0; i < mat.size(); ++i) {
    mat_.row(i) =
        Eigen::Map<Eigen::VectorXf>(mat.at(i).data(), mat.at(i).size());
  }

  return mat_;
}

Vector LinearAlgebraUtil::SubtractVector(Vector v1, Vector v2) {
  Eigen::Map<Eigen::VectorXf> v1_(v1.data(), v1.size());
  Eigen::Map<Eigen::VectorXf> v2_(v2.data(), v2.size());

  v1_ -= v2_;
  v1 = MatrixXfToVector(v1_);

  return v1;
}

Vector LinearAlgebraUtil::MultiplyMatrixVector(Matrix mat, Vector v) {
  Eigen::MatrixXf mat_ = MatrixToMatrixXf(mat);
  Eigen::Map<Eigen::VectorXf> v_(v.data(), v.size());

  Eigen::MatrixXf result_ = mat_ * v_;
  Vector result = MatrixXfToVector(result_);

  return result;
}

Vector LinearAlgebraUtil::AddVectorScalar(Vector v, float a) {
  Eigen::Map<Eigen::VectorXf> v_(v.data(), v.size());

  auto array_ = v_.array();
  array_ += a;
  v_ = array_.matrix();
  v = MatrixXfToVector(v_);

  return v;
}

Vector LinearAlgebraUtil::AddVectors(Vector v1, Vector v2) {
  Eigen::Map<Eigen::VectorXf> v1_(v1.data(), v1.size());
  Eigen::Map<Eigen::VectorXf> v2_(v2.data(), v2.size());

  v1_ += v2_;
  v1 = MatrixXfToVector(v1_);

  return v1;
}

Vector LinearAlgebraUtil::MultiplyVectorScalar(Vector v, float a) {
  Eigen::Map<Eigen::VectorXf> v_(v.data(), v.size());

  auto array_ = v_.array();
  array_ *= a;
  v_ = array_.matrix();
  v = MatrixXfToVector(v_);

  return v;
}

Matrix LinearAlgebraUtil::TransposeMatrix(Matrix mat) {
  Eigen::MatrixXf mat_ = MatrixToMatrixXf(mat);

  mat_.transposeInPlace();
  Matrix matT = MatrixXfToMatrix(mat_);

  return matT;
}

}  // namespace brave_federated
