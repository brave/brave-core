/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account_handler.h"

#include <string>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "components/password_manager/core/browser/ui/weak_check_utility.h"

BraveAccountHandler::BraveAccountHandler() = default;

BraveAccountHandler::~BraveAccountHandler() = default;

void BraveAccountHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getPasswordStrength",
      base::BindRepeating(&BraveAccountHandler::GetPasswordStrength,
                          base::Unretained(this)));
}

void BraveAccountHandler::GetPasswordStrength(const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);
  auto* password = args[1].GetIfString();
  CHECK(password);

  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(password_manager::GetPasswordStrength(*password)));
}
