/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_SYNC_CREDENTIALS_FILTER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_SYNC_CREDENTIALS_FILTER_H_

#include "components/password_manager/core/browser/credentials_filter.h"

#define ShouldSave                                         \
  ShouldSave_ChromiumImpl(const PasswordForm& form) const; \
  bool ShouldSave

#include "../../../../../../components/password_manager/core/browser/sync_credentials_filter.h"

#undef ShouldSave

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_SYNC_CREDENTIALS_FILTER_H_
