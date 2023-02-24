/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_BRAVE_SERVICES_KEY_HELPER_H_
#define BRAVE_COMPONENTS_CONSTANTS_BRAVE_SERVICES_KEY_HELPER_H_

#include "base/strings/string_piece.h"
#include "brave/components/constants/brave_services_key.h"

class GURL;

namespace brave {

constexpr bool CanUseBraveServices() {
  return !base::StringPiece(BUILDFLAG(BRAVE_SERVICES_KEY)).empty();
}

bool ShouldAddBraveServicesKeyHeader(const GURL& url);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_CONSTANTS_BRAVE_SERVICES_KEY_HELPER_H_
