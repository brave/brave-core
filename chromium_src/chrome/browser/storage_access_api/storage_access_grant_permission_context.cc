/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_STORAGE_ACCESS_GRANT_PERMISSION_CONTEXT_CHECK_FOR_AUTO_GRANT_OR_AUTO_DENIAL \
  NotifyPermissionSetInternal(                                                            \
      request_data.id, request_data.requesting_origin,                                    \
      request_data.embedding_origin, std::move(callback), /*persist=*/true,               \
      CONTENT_SETTING_BLOCK, RequestOutcome::kDeniedByFirstPartySet);                     \
  return;

#include "src/chrome/browser/storage_access_api/storage_access_grant_permission_context.cc"
#undef BRAVE_STORAGE_ACCESS_GRANT_PERMISSION_CONTEXT_CHECK_FOR_AUTO_GRANT_OR_AUTO_DENIAL
