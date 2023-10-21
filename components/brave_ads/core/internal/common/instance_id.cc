/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/instance_id.h"

#include "base/no_destructor.h"
#include "base/uuid.h"

namespace brave_ads {

const std::string& GetInstanceId() {
  static const base::NoDestructor<std::string> kInstanceId(
      base::Uuid::GenerateRandomV4().AsLowercaseString());
  return *kInstanceId;
}

}  // namespace brave_ads
