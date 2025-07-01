/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_origin/brave_origin_handler.h"

#include <memory>

#include "base/check.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/task/thread_pool.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"

BraveOriginHandler::BraveOriginHandler(Profile* profile) : profile_(profile) {
  // TODO: put pref watcher on anything if there's a way to change the value
  // besides group policy.
}

BraveOriginHandler::~BraveOriginHandler() = default;

void BraveOriginHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getInitialState",
      base::BindRepeating(&BraveOriginHandler::HandleGetInitialState,
                          base::Unretained(this)));
}

void BraveOriginHandler::HandleGetInitialState(
    const base::Value::List& args) {
  AllowJavascript();

  // TODO(bsclifton): look at receipt, etc.
  bool brave_origin_enabled = true;
  // TODO(bsclifton): look at real preference values.
  bool toggle_ntp_ads = false;
  bool toggle_rewards = false;
  bool toggle_search_ads = false;
  bool toggle_email_alias = false;
  bool toggle_leo_ai = false;
  bool toggle_news = false;
  bool toggle_p3a_crash_report = false;
  bool toggle_sidebar = false;
  bool toggle_tor_windows = false;
  bool toggle_vpn = false;
  bool toggle_wallet = false;
  bool toggle_web3domains = false;

  base::Value::Dict initial_state;

  initial_state.Set("enabled", brave_origin_enabled);
  initial_state.Set("toggle_ntp_ads", toggle_ntp_ads);
  initial_state.Set("toggle_rewards", toggle_rewards);
  initial_state.Set("toggle_search_ads", toggle_search_ads);
  initial_state.Set("toggle_email_alias", toggle_email_alias);
  initial_state.Set("toggle_leo_ai", toggle_leo_ai);
  initial_state.Set("toggle_news", toggle_news);
  initial_state.Set("toggle_p3a_crash_report", toggle_p3a_crash_report);
  initial_state.Set("toggle_sidebar", toggle_sidebar);
  initial_state.Set("toggle_tor_windows", toggle_tor_windows);
  initial_state.Set("toggle_vpn", toggle_vpn);
  initial_state.Set("toggle_wallet", toggle_wallet);
  initial_state.Set("toggle_web3domains", toggle_web3domains);

  ResolveJavascriptCallback(
      args[0],
      initial_state);
}
