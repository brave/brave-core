/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../chrome/browser/permissions/chrome_permissions_client.cc"

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#endif

bool ChromePermissionsClient::BraveCanBypassEmbeddingOriginCheck(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    ContentSettingsType type) {
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  // Since requesting_origin has been overwritten by us to add address info,
  // it will fail Chromium's origin check because requesting_origin is now
  // different from the embedding_origin. To address this, we get the original
  // requesting origin back and use it to check with embedding_origin instead,
  // and let it bypass the origin check from Chromium when the original
  // requesting_origin & embedding_origin are the same.
  std::string original_requesting_origin;
  if (type == ContentSettingsType::BRAVE_ETHEREUM &&
      brave_wallet::ParseRequestingOriginFromSubRequest(
          requesting_origin, &original_requesting_origin, nullptr) &&
      GURL(original_requesting_origin) == embedding_origin) {
    return true;
  }
#endif

  return CanBypassEmbeddingOriginCheck(requesting_origin, embedding_origin);
}
