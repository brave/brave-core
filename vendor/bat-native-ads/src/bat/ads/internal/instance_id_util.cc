/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/instance_id_util.h"

#include "base/guid.h"
#include "base/no_destructor.h"

namespace ads {

const std::string& GetInstanceId() {
  static const base::NoDestructor<std::string> guid(
      base::GUID::GenerateRandomV4().AsLowercaseString());
  return *guid;
}

}  // namespace ads
