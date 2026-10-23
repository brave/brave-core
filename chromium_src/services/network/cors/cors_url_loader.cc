/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"
#include "base/types/optional_ref.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

// A cross-origin request initiated by a .onion page must not disclose the
// address it came from, so it is handled as if its origin were tainted.
bool IsCrossOriginOnionRequest(base::optional_ref<const url::Origin> initiator,
                               const GURL& url) {
  return initiator &&
         base::EndsWith(initiator->host(), ".onion",
                        base::CompareCase::INSENSITIVE_ASCII) &&
         !initiator->IsSameOriginWith(url);
}

}  // namespace

#include <services/network/cors/cors_url_loader.cc>
