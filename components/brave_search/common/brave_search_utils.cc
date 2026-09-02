// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/brave_search_utils.h"

#include <string_view>
#include <vector>

#include "base/feature_list.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_search/common/features.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

namespace {

const std::vector<url::Origin>& OriginList() {
  static const base::NoDestructor<std::vector<url::Origin>> list([] {
    std::vector<url::Origin> list;
    std::ranges::transform(brave_search::kVettedHosts, std::back_inserter(list),
                           [](auto& origin_string) {
                             return url::Origin::Create(GURL(origin_string));
                           });
    return list;
  }());
  return *list;
}

}  // namespace

namespace brave_search {

bool IsAllowedHost(const GURL& origin) {
  if (!origin.is_valid() || !origin.SchemeIs(url::kHttpsScheme)) {
    return false;
  }
  const auto& safe_origins = OriginList();
  for (const url::Origin& safe_origin : safe_origins) {
    if (safe_origin.host() == origin.host()) {
      return true;
    }
  }
  return false;
}

bool IsDefaultAPIEnabled() {
  return base::FeatureList::IsEnabled(
      brave_search::features::kBraveSearchDefaultAPIFeature);
}

GURL OverrideWithNewTabSource(GURL url,
                              PrefService* local_state,
                              bool is_first_run) {
  std::string_view source = "newtab";
  if (features::IsSearchNewTabV1SourceEnabled()) {
    source = "newtab_v1";
  }
#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::features::IsShowAIChatInputOnNewTabPageEnabled(local_state,
                                                              is_first_run)) {
    source = "newtab_v2";
  }
#endif
  return net::AppendOrReplaceQueryParameter(url, "source", source);
}

}  // namespace brave_search
