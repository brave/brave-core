/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define ShouldExposeExperimentalAuthenticationKey \
  DISABLED_ShouldExposeExperimentalAuthenticationKey
#include "../../../../../../../chrome/browser/sync/test/integration/single_client_custom_passphrase_sync_test.cc"
#undef ShouldExposeExperimentalAuthenticationKey
