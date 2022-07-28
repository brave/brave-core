// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_TEST_UTIL_H_
#define BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_TEST_UTIL_H_

#include <string>

#include "services/network/public/cpp/resource_request.h"

namespace brave {

std::string HandleRandomnessRequest(const network::ResourceRequest& request,
                                    uint8_t expected_epoch);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_TEST_UTIL_H_
