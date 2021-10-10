/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_wallet_handler.h"

#include "base/bind.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"

#include "brave/components/brave_wallet/browser/pref_names.h"

void BraveWalletHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getAutoLockMinutes",
      base::BindRepeating(&BraveWalletHandler::GetAutoLockMinutes,
                          base::Unretained(this)));
}

void BraveWalletHandler::GetAutoLockMinutes(base::Value::ConstListView args) {
  CHECK_EQ(args.size(), 1U);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(prefs->GetInteger(kBraveWalletAutoLockMinutes)));
}
