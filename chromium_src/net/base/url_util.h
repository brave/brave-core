// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_URL_UTIL_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_URL_UTIL_H_

#include "src/net/base/url_util.h"

#include "net/base/net_export.h"
#include "url/origin.h"

namespace net {

// Convert this URL into an encoded storage domain string, which is used
// to identify a particular storage domain uniquely in a BrowserContext.
NET_EXPORT std::string URLToEphemeralStorageDomain(const GURL& url);

// Helpers to access Origin internal data to use for Ephemeral Storage.
class NET_EXPORT EphemeralStorageOriginUtils {
 public:
  /* Checks if an origin is opaque and nonce is initialized. */
  static bool CanUseNonceForEphemeralStorageKeying(const url::Origin& origin);
  /* Returns nonce to use as an Ephemeral Storage key. CHECKs if an origin
   * cannot be used for Ephemeral Storage keying. */
  static const base::UnguessableToken& GetNonceForEphemeralStorageKeying(
      const url::Origin& origin);
};

}  // namespace net

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_URL_UTIL_H_
