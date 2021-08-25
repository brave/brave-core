/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/password_manager/core/browser/sync_credentials_filter.h"

#define ShouldSave ShouldSave_ChromiumImpl
#include "../../../../../../components/password_manager/core/browser/sync_credentials_filter.cc"
#undef ShouldSave

namespace password_manager {

bool SyncCredentialsFilter::ShouldSave(const PasswordForm& form) const {
  bool should_save = ShouldSave_ChromiumImpl(form);
  if (!should_save && sync_util::IsGaiaCredentialPage(form.signon_realm)) {
    return true;
  }
  return should_save;
}

}  // namespace password_manager
