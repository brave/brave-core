/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FEATURES_FEATURES_H_
#define BAT_ADS_INTERNAL_FEATURES_FEATURES_H_

#include "base/optional.h"

namespace ads {
namespace features {

bool HasActiveStudy();

base::Optional<std::string> GetStudy();

base::Optional<std::string> GetGroup();

void Log();

}  // namespace features
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FEATURES_FEATURES_H_
