// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/brave_domains/constants.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "url/url_constants.h"

namespace brave_domains {

namespace {
constexpr char kGate3DomainPrefix[] = "gate3.wallet";
}  // namespace

std::string GetGate3URL() {
  return base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                       GetServicesDomain(kGate3DomainPrefix)});
}

}  // namespace brave_domains
