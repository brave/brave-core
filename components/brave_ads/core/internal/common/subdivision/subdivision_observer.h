/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_SUBDIVISION_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_SUBDIVISION_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace brave_ads {

class SubdivisionObserver : public base::CheckedObserver {
 public:
  // Invoked when the subdivision has updated.
  virtual void OnDidUpdateSubdivision(const std::string& subdivision) {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_SUBDIVISION_OBSERVER_H_
