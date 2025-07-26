// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/webui/plural_string_handler.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
// #include "chrome/grit/generated_resources.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

PluralStringHandler::PluralStringHandler() = default;

PluralStringHandler::~PluralStringHandler() = default;

void PluralStringHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getPluralString",
      base::BindRepeating(&PluralStringHandler::HandleGetPluralString,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getPluralStringTupleWithComma",
      base::BindRepeating(
          &PluralStringHandler::HandleGetPluralStringTupleWithComma,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getPluralStringTupleWithPeriods",
      base::BindRepeating(
          &PluralStringHandler::HandleGetPluralStringTupleWithPeriods,
          base::Unretained(this)));
}

void PluralStringHandler::AddLocalizedString(const std::string& name, int id) {
  name_to_id_[name] = id;
}

void PluralStringHandler::HandleGetPluralString(const base::Value::List& args) {
  CHECK_EQ(3U, args.size());

  const base::Value& callback_id = args[0];
  const std::string& message_name = args[1].GetString();
  int count = args[2].GetInt();

  auto string = GetPluralizedStringForMessageName(message_name, count);

  web_ui()->ResolveJavascriptCallback(callback_id, base::Value(string));
}

void PluralStringHandler::HandleGetPluralStringTupleWithComma(
    const base::Value::List& args) {
  GetPluralStringTuple(args, IDS_CONCAT_TWO_STRINGS_WITH_COMMA);
}

void PluralStringHandler::HandleGetPluralStringTupleWithPeriods(
    const base::Value::List& args) {
  GetPluralStringTuple(args, IDS_CONCAT_TWO_STRINGS_WITH_PERIODS);
}

void PluralStringHandler::GetPluralStringTuple(const base::Value::List& args,
                                               int string_tuple_id) {
  CHECK_EQ(5U, args.size());

  const base::Value& callback_id = args[0];
  const std::string& message_name1 = args[1].GetString();
  int count1 = args[2].GetInt();
  const std::string& message_name2 = args[3].GetString();
  int count2 = args[4].GetInt();

  auto string1 = GetPluralizedStringForMessageName(message_name1, count1);
  auto string2 = GetPluralizedStringForMessageName(message_name2, count2);

  web_ui()->ResolveJavascriptCallback(
      callback_id, base::Value(l10n_util::GetStringFUTF8(string_tuple_id,
                                                         string1, string2)));
}

std::u16string PluralStringHandler::GetPluralizedStringForMessageName(
    std::string message_name,
    int count) {
  auto message_id_it = name_to_id_.find(message_name);
  CHECK(name_to_id_.end() != message_id_it);
  return l10n_util::GetPluralStringFUTF16(message_id_it->second, count);
}
