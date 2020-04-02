/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "brave/components/client_model_parameters/browser/client_model_parameter_service.h"

// npm run test -- brave_unit_tests --filter=ClientModelParameterService*

namespace client_model_parameters {

TEST(ClientModelParameterServiceTest, EmptyModel) {
  ClientModelParameterService service;
  const std::string model = "";

  EXPECT_EQ(service.GetParameters(model), "");
}

}  // namespace client_model_parameters
