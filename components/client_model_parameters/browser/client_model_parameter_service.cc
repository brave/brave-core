/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/client_model_parameters/browser/client_model_parameter_service.h"

namespace client_model_parameters {

ClientModelParameterService::ClientModelParameterService() = default;
ClientModelParameterService::~ClientModelParameterService() = default;

std::string ClientModelParameterService::GetParameters(
    const std::string& model) {
  std::string parameters_json;
  if (model == "foobar_classifier") {
    parameters_json = R"({"foo":"bar"})";
  } else {
    parameters_json = "";
  }

  return parameters_json;
}

}  // namespace client_model_parameters
