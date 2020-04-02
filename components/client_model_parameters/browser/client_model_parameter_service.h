/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_CLIENT_MODEL_PARAMETERS_CLIENT_MODEL_PARAMETER_SERVICE_H_
#define COMPONENTS_CLIENT_MODEL_PARAMETERS_CLIENT_MODEL_PARAMETER_SERVICE_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace client_model_parameters {

class ClientModelParameterService {
 public:
  ClientModelParameterService();
  ~ClientModelParameterService();

  std::string GetParameters(
      const std::string& model);
};

}  // namespace client_model_parameters

#endif  // COMPONENTS_CLIENT_MODEL_PARAMETERS_CLIENT_MODEL_PARAMETER_SERVICE_H_
