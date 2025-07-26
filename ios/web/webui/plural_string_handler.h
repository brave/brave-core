// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_WEBUI_PLURAL_STRING_HANDLER_H_
#define BRAVE_IOS_WEB_WEBUI_PLURAL_STRING_HANDLER_H_

#include <map>
#include <string>

#include "base/values.h"
#include "ios/web/public/webui/web_ui_ios_message_handler.h"

// A handler which provides pluralized strings.
class PluralStringHandler : public web::WebUIIOSMessageHandler {
 public:
  PluralStringHandler();

  PluralStringHandler(const PluralStringHandler&) = delete;
  PluralStringHandler& operator=(const PluralStringHandler&) = delete;

  ~PluralStringHandler() override;

  void AddLocalizedString(const std::string& name, int id);

  // WebUIMessageHandler:
  void RegisterMessages() override;

 private:
  void HandleGetPluralString(const base::Value::List& args);

  // Constructs two pluralized strings from the received arguments for the two
  // strings, and then concatenates those with comma and whitespace in between.
  void HandleGetPluralStringTupleWithComma(const base::Value::List& args);

  // Constructs two pluralized strings from the received arguments for the two
  // strings, and then concatenates those with period and whitespace in between,
  // and a period afterwards.
  void HandleGetPluralStringTupleWithPeriods(const base::Value::List& args);

  // Constructs two pluralized strings from the received arguments for the two
  // strings, and then concatenates those using the concatenation template
  // specified. This method should only be called from within the
  // |HandleGetPluralStringTuple*| methods above.
  void GetPluralStringTuple(const base::Value::List& args, int string_tuple_id);

  std::u16string GetPluralizedStringForMessageName(std::string message_name,
                                                   int count);

  std::map<std::string, int> name_to_id_;
};

#endif  // BRAVE_IOS_WEB_WEBUI_PLURAL_STRING_HANDLER_H_
