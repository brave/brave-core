/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// We don't have one client with scrypt and another using PBKDF2
#define TwoClientCustomPassphraseSyncTestScryptEnabledInPreTest \
  DISABLED_TwoClientCustomPassphraseSyncTestScryptEnabledInPreTest
#include "../../../../../../../chrome/browser/sync/test/integration/two_client_custom_passphrase_sync_test.cc"
#undef TwoClientCustomPassphraseSyncTestScryptEnabledInPreTest
