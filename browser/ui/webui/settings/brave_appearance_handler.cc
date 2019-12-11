/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/web_ui.h"

BraveAppearanceHandler::BraveAppearanceHandler() {
  local_state_change_registrar_.Init(g_browser_process->local_state());
  local_state_change_registrar_.Add(
      kBraveDarkMode,
      base::Bind(&BraveAppearanceHandler::OnBraveDarkModeChanged,
                 base::Unretained(this)));
}

BraveAppearanceHandler::~BraveAppearanceHandler() = default;

void BraveAppearanceHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "setBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::SetBraveThemeType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::GetBraveThemeType,
                          base::Unretained(this)));
}

void BraveAppearanceHandler::SetBraveThemeType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  AllowJavascript();

  int int_type;
  args->GetInteger(0, &int_type);
  dark_mode::SetBraveDarkModeType(
      static_cast<dark_mode::BraveDarkModeType>(int_type));
}

void BraveAppearanceHandler::GetBraveThemeType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  AllowJavascript();
  // GetBraveThemeType() should be used because settings option displays all
  // available options including default.
  ResolveJavascriptCallback(
      args->GetList()[0],
      base::Value(static_cast<int>(dark_mode::GetBraveDarkModeType())));
}

void BraveAppearanceHandler::OnBraveDarkModeChanged() {
  // GetBraveThemeType() should be used because settings option displays all
  // available options including default.
  if (IsJavascriptAllowed()) {
    FireWebUIListener(
        "brave-theme-type-changed",
        base::Value(static_cast<int>(dark_mode::GetBraveDarkModeType())));
  }
}
