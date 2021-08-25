/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_SERVER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_SERVER_UTIL_H_

#include <string>

namespace brave_adaptive_captcha {

std::string GetServerUrl(const std::string& path);

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_SERVER_UTIL_H_
