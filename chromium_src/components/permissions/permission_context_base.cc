/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_context_base.h"

#include "components/permissions/permissions_client.h"

#define CanBypassEmbeddingOriginCheck(REQUESTING_ORIGIN, EMBEDDING_ORIGIN) \
  BraveCanBypassEmbeddingOriginCheck(REQUESTING_ORIGIN, EMBEDDING_ORIGIN,  \
                                     content_settings_type_)

#include "src/components/permissions/permission_context_base.cc"
#undef CanBypassEmbeddingOriginCheck
