/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SIGNIN_INTERNAL_IDENTITY_MANAGER_PRIMARY_ACCOUNT_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SIGNIN_INTERNAL_IDENTITY_MANAGER_PRIMARY_ACCOUNT_MANAGER_H_

#define RevokeSyncConsent                                    \
  RevokeSyncConsent_ChromiumImpl(                            \
      signin_metrics::ProfileSignout signout_source_metric); \
  void RevokeSyncConsent

#include "src/components/signin/internal/identity_manager/primary_account_manager.h"  // IWYU pragma: export

#undef RevokeSyncConsent

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SIGNIN_INTERNAL_IDENTITY_MANAGER_PRIMARY_ACCOUNT_MANAGER_H_
