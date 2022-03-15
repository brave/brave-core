/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_OBSERVER_H_

#include "base/observer_list_types.h"

namespace brave_federated {

class Observer : public base::CheckedObserver {
 public:
  virtual void OnEligibilityChanged(bool is_eligible) = 0;

 protected:
  ~Observer() override = default;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_OBSERVER_H_
