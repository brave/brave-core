/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account_row_handler.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/webui/brave_account/brave_account_ui_desktop.h"
#include "brave/components/brave_account/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace brave_account {

BraveAccountRowHandler::BraveAccountRowHandler(
    mojo::PendingReceiver<mojom::BraveAccountRowHandler> row_handler,
    mojo::PendingRemote<mojom::BraveAccountRow> row,
    content::WebUI* web_ui)
    : row_handler_(this, std::move(row_handler)),
      row_(std::move(row)),
      web_ui_(&CHECK_DEREF(web_ui)),
      pref_service_(CHECK_DEREF(Profile::FromWebUI(web_ui_)).GetPrefs()) {
  CHECK(pref_service_);
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.AddMultiple(
      {prefs::kVerificationToken, prefs::kAuthenticationToken},
      base::BindRepeating(&BraveAccountRowHandler::OnPrefChanged,
                          base::Unretained(this)));
  OnPrefChanged();
}

BraveAccountRowHandler::~BraveAccountRowHandler() = default;

void BraveAccountRowHandler::OpenDialog() {
  ShowBraveAccountDialog(web_ui_);
}

void BraveAccountRowHandler::OnPrefChanged() {
  if (!pref_service_->GetString(prefs::kAuthenticationToken).empty()) {
    return row_->OnAuthenticationTokenChanged();
  }

  if (!pref_service_->GetString(prefs::kVerificationToken).empty()) {
    return row_->OnVerificationTokenChanged();
  }
}

}  // namespace brave_account
