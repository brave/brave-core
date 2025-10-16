/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/service/sync_service_utils.h"

namespace syncer {
bool RejectConsent() {
  return true;
}
}  // namespace syncer

#define GetUploadToGoogleState(...) \
  RejectConsent() || syncer::GetUploadToGoogleState(__VA_ARGS__)

#include <components/page_image_service/image_service_consent_helper.cc>

#undef GetUploadToGoogleState
