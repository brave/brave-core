/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/media/router/discovery/dial/dial_service_impl.h"

#include "components/version_info/version_info.h"

namespace version_info {
constexpr base::StringPiece GetProductNameForChrome() {
  return "Google Chrome";
}
}  // namespace version_info

#define GetProductName GetProductNameForChrome
#include "src/chrome/browser/media/router/discovery/dial/dial_service_impl.cc"
#undef GetProductName
