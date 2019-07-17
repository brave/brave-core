/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define CHECK_SYNC_POLICY \
  if (sync_service && sync_service->HasDisableReason( \
        syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY)) \
        return false;

#include "../../../../../../../chrome/browser/ui/autofill/payments/save_card_bubble_controller_impl.cc"  // NOLINT
#undef CHECK_SYNC_POLICY

