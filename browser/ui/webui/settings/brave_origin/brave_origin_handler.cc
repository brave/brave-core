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
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "brave/components/brave_rewards/core/pref_names.h"
#endif

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
  web_ui()->RegisterMessageCallback(
      "setRewardsEnabled",
      base::BindRepeating(&BraveOriginHandler::SetRewardsEnabled,
                          base::Unretained(this)));
}

void BraveOriginHandler::HandleGetInitialState(
    const base::Value::List& args) {
  AllowJavascript();

  base::Value::Dict initial_state;

  // TODO(bsclifton): look at receipt, etc.
  initial_state.Set("enabled", true);

  initial_state.Set("rewards_enabled",
                    !profile_->GetPrefs()->GetBoolean(
                        brave_rewards::prefs::kDisabledByPolicy));

  // TODO(bsclifton): look at real preference values.
  initial_state.Set("toggle_search_ads", false);
  initial_state.Set("toggle_email_alias", false);
  initial_state.Set("toggle_p3a_crash_report", false);
  initial_state.Set("toggle_sidebar", false);
  initial_state.Set("toggle_web3domains", false);

  ResolveJavascriptCallback(
      args[0],
      initial_state);
}

void BraveOriginHandler::SetRewardsEnabled(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  bool enabled = args[0].GetBool();
  LOG(ERROR) << "BSC]] GOT enabled=" << enabled;

  profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kDisabledByPolicy,
                                   enabled);
  AllowJavascript();
}
