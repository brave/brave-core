/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_REGIONS_H_
#define BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_REGIONS_H_

#include <string>
#include <vector>

#include "base/no_destructor.h"

namespace crypto_dot_com {

const base::NoDestructor<std::vector<std::string>> unsupported_regions(
    {"JP", "DZ", "BD", "BO", "EC", "EG", "ID", "MA", "NP", "PK", "AE", "VN"});

}  // namespace crypto_dot_com

#endif  // BRAVE_COMPONENTS_CRYPTO_DOT_COM_BROWSER_REGIONS_H_
