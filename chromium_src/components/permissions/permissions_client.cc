/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../components/permissions/permissions_client.cc"

namespace permissions {

bool PermissionsClient::BraveCanBypassEmbeddingOriginCheck(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    ContentSettingsType type) {
  return CanBypassEmbeddingOriginCheck(requesting_origin, embedding_origin);
}

}  // namespace permissions
