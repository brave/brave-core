// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_talk/common/brave_talk_utils.h"

#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/strings/string_piece_forward.h"
#include "brave/components/brave_search/common/features.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

constexpr auto kVettedHosts = base::MakeFixedFlatSet<base::StringPiece>(
    {"talk.brave.com", "ringed-ubiquitous-wisteria.glitch.me"});

}  // namespace

namespace brave_talk {

bool IsAllowedHost(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }
  std::string host = url.host();
  return base::Contains(kVettedHosts, host);
}

bool IsDefaultAPIEnabled() {
    return true; // TODO: Add Feature Flag
}

}  // namespace brave_search