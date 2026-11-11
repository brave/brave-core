/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/url_request/url_request_job.h"

#include "base/strings/string_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {

namespace {

bool IsCrossOriginOnionReferrer(const GURL& original_referrer,
                                const GURL& destination) {
  return base::EndsWith(original_referrer.host(), ".onion",
                        base::CompareCase::INSENSITIVE_ASCII) &&
         !url::IsSameOriginWith(original_referrer, destination);
}

}  // namespace

}  // namespace net

#include <net/url_request/url_request_job.cc>
