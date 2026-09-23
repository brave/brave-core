/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_PRIVATE_CDN_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_PRIVATE_CDN_HELPER_H_

#include <string_view>

namespace brave::private_cdn {

bool RemovePadding(std::string_view* padded_string);

}  // namespace brave::private_cdn

#endif  // BRAVE_COMPONENTS_BRAVE_PRIVATE_CDN_PRIVATE_CDN_HELPER_H_
