// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/passwords/brave_web_view_password_manager_client.h"

#include "ios/chrome/browser/shared/model/application_context/application_context.h"

bool BraveWebViewPasswordManagerClient::PromptUserToSaveOrUpdatePassword(
    std::unique_ptr<password_manager::PasswordFormManagerForUI> form_to_save,
    bool update_password) {
  // This override removes the account storage check since our version can use
  // profile stored passwords

  if (form_to_save->IsBlocklisted()) {
    return false;
  }

  if (update_password) {
    [bridge_ showUpdatePasswordInfoBar:std::move(form_to_save) manual:NO];
  } else {
    [bridge_ showSavePasswordInfoBar:std::move(form_to_save) manual:NO];
  }

  return true;
}

PrefService* BraveWebViewPasswordManagerClient::GetLocalStatePrefs() const {
  return GetApplicationContext()->GetLocalState();
}
