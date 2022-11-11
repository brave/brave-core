/*************************************************************************************************
 * Copyright (c) 2022 The Flower Authors.
 *
 * @file client.h
 *
 * @brief C++ Flower client (abstract base class)
 *
 * @author Lekang Jiang
 *
 * @version 1.0
 *
 * @date 03/09/2021
 *
 *************************************************************************************************/

#ifndef BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_CLIENT_H_
#define BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_CLIENT_H_

#include "brave/third_party/flower/src/cc/flwr/include/typing.h"

namespace flower {
/**
 *
 * Abstract base class for C++ Flower clients
 *
 */
class Client {
 public:
  /**
   *
   * @brief Return the current local model parameters
   * @return ParametersRes
   *             The current local model parameters
   *
   */
  virtual ParametersRes GetParameters() = 0;

  virtual PropertiesRes GetProperties(PropertiesIns ins) = 0;

  virtual bool IsCommunicating() = 0;
  /**
   *
   * @brief Refine the provided weights using the locally held dataset
   * @param FitIns
   *             The training instructions containing (global) model parameters
   *             received from the server and a dictionary of configuration
   * values used to customize the local training process.
   * @return FitRes
   *             The training result containing updated parameters and other
   * details such as the number of local training examples used for training.
   */
  virtual FitRes Fit(FitIns ins) = 0;

  /**
   *
   * @brief Evaluate the provided weights using the locally held dataset.
   * @param EvaluateIns
   *             The evaluation instructions containing (global) model
   * parameters received from the server and a dictionary of configuration
   * values used to customize the local evaluation process.
   * @return EvaluateRes
   *             The evaluation result containing the loss on the local dataset
   * and other details such as the number of local data examples used for
   *             evaluation.
   */
  virtual EvaluateRes Evaluate(EvaluateIns ins) = 0;
};

}  // namespace flower

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_CLIENT_H_
