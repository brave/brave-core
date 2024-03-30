/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/discovery/dial/dial_service_impl.h"

#include "brave/components/version_info/version_info.h"
#include "components/version_info/version_info.h"

namespace version_info {
constexpr std::string_view GetProductNameForChrome() {
  return "Google Chrome";
}
}  // namespace version_info

// We want to match Chrome behaviour for M-SEARCH USER-AGENT string.
#define GetProductName GetProductNameForChrome
#define GetVersionNumber GetBraveChromiumVersionNumber
#include "src/chrome/browser/media/router/discovery/dial/dial_service_impl.cc"
#undef GetProductName
#undef GetVersionNumber
