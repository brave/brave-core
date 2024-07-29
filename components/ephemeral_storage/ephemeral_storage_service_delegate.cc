/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service_delegate.h"

namespace ephemeral_storage {

bool EphemeralStorageServiceDelegate::DoesProfileHaveAnyBrowserWindow() const {
  return false;
}

}  // namespace ephemeral_storage
