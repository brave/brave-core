/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/default_brave_shields_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"

using brave_shields::ControlType;
using brave_shields::ControlTypeFromString;
using brave_shields::ControlTypeToString;

void DefaultBraveShieldsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "getAdControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetAdControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setAdControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetAdControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getCookieControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::GetCookieControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setCookieControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetCookieControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getFingerprintingControlType",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::GetFingerprintingControlType,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setFingerprintingControlType",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetFingerprintingControlType,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setHTTPSEverywhereEnabled",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::SetHTTPSEverywhereEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setNoScriptControlType",
      base::BindRepeating(&DefaultBraveShieldsHandler::SetNoScriptControlType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "resetShieldsSettings",
      base::BindRepeating(
          &DefaultBraveShieldsHandler::ResetShieldsSettings,
          base::Unretained(this)));
}

void DefaultBraveShieldsHandler::GetAdControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetAdControlType(profile_, GURL());

  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0].Clone(),
      base::Value(setting == ControlType::ALLOW));
}

void DefaultBraveShieldsHandler::SetAdControlType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  brave_shields::SetAdControlType(profile_,
                                  value ? ControlType::BLOCK
                                        : ControlType::ALLOW,
                                  GURL());
}

void DefaultBraveShieldsHandler::GetCookieControlType(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  ControlType setting = brave_shields::GetCookieControlType(profile_, GURL());

  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0].Clone(),
      base::Value(ControlTypeToString(setting)));
}

void DefaultBraveShieldsHandler::SetCookieControlType(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  std::string value;
  args->GetString(0, &value);

  brave_shields::SetCookieControlType(profile_,
                                      ControlTypeFromString(value),
                                      GURL());
}

void DefaultBraveShieldsHandler::GetFingerprintingControlType(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);

  ControlType setting =
      brave_shields::GetFingerprintingControlType(profile_, GURL());

  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0].Clone(),
      base::Value(ControlTypeToString(setting)));
}

void DefaultBraveShieldsHandler::SetFingerprintingControlType(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  std::string value;
  args->GetString(0, &value);

  brave_shields::SetFingerprintingControlType(profile_,
                                              ControlTypeFromString(value),
                                              GURL());
}

void DefaultBraveShieldsHandler::SetHTTPSEverywhereEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  brave_shields::SetHTTPSEverywhereEnabled(profile_,
                                           value,
                                           GURL());
}

void DefaultBraveShieldsHandler::SetNoScriptControlType(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool value;
  args->GetBoolean(0, &value);

  brave_shields::SetNoScriptControlType(profile_,
                                        value ? ControlType::BLOCK
                                              : ControlType::ALLOW,
                                        GURL());
}

void DefaultBraveShieldsHandler::ResetShieldsSettings(
    const base::ListValue* args) {
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_);
  static_cast<BraveHostContentSettingsMap*>(map)->ClearSettingsForPluginsType(
      base::Time(), base::Time(), true /* is_shields */);

  AllowJavascript();
  ResolveJavascriptCallback(*callback_id, base::Value() /* Promise<void> */);
}
