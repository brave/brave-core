/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/signin/internal/identity_manager/primary_account_manager.h"

#define RevokeSyncConsent RevokeSyncConsent_ChromiumImpl
#include "src/components/signin/internal/identity_manager/primary_account_manager.cc"
#undef RevokeSyncConsent

void PrimaryAccountManager::RevokeSyncConsent(
    signin_metrics::ProfileSignout source_metric) {}
