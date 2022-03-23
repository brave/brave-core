/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_

#include <memory>
#include <vector>

namespace brave_federated {

class Model {
 public:
  Model();
  ~Model();

  void Forward(std::vector<float> input);

 private:
  std::vector<std::vector<float>> parameters_;
};

} // namespace brave_federated

#endif //BRAVE_COMPONENTS_BRAVE_FEDERATED_CLIENT_MODEL_H_