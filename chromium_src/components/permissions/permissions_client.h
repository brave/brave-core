/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSIONS_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSIONS_CLIENT_H_

#define CanBypassEmbeddingOriginCheck                               \
  BraveCanBypassEmbeddingOriginCheck(const GURL& requesting_origin, \
                                     const GURL& embedding_origin,  \
                                     ContentSettingsType type);     \
  virtual bool CanBypassEmbeddingOriginCheck

#include "../../../../components/permissions/permissions_client.h"
#undef CanBypassEmbeddingOriginCheck

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSIONS_CLIENT_H_
