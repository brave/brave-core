/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/utils.h"

#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

bool IsBraveSearchURL(const GURL& url) {
  return url.is_valid() && url.SchemeIs(url::kHttpsScheme) &&
         url.host_piece() ==
             brave_domains::GetServicesDomain(kBraveSearchURLPrefix);
}

bool IsOpenLeoButtonFromBraveSearchURL(const GURL& url) {
  return IsBraveSearchURL(url) && url.path_piece() == "/leo";
}

}  // namespace ai_chat
