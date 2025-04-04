// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_opeartion_context.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "url/url_util.h"

namespace psst {

// static
std::unique_ptr<PsstOperationContext> PsstOperationContext::LoadContext(
    const base::Value& user_script_result,
    const MatchedRule& rule) {
  if (!user_script_result.is_dict()) {
    LOG(INFO) << "[PSST] LoadContext #100";
    return nullptr;
  }

  const auto* params = user_script_result.GetIfDict();

  std::optional<std::string> user_id;
  if (const std::string* parsed_user_id = params->FindString("user")) {
    user_id = *parsed_user_id;
  }

  std::optional<std::string> share_experience_link;
  if (const std::string* parsed_share_experience_link =
          params->FindString("share_experience_link")) {
    share_experience_link = *parsed_share_experience_link;
  }

  if (!user_id || !share_experience_link) {
    LOG(INFO) << "[PSST] LoadContext #200 user_id:" << user_id.has_value()
              << " share_experience_link:" << share_experience_link.has_value()
              << " rule_name:" << rule.Name();
    return nullptr;
  }

  return std::unique_ptr<PsstOperationContext>(
      new PsstOperationContext(*user_id, *share_experience_link, rule));
}

PsstOperationContext::PsstOperationContext(
    const std::string& user_id,
    const std::string& share_experience_link,
    const MatchedRule& rule)
    : user_id_(user_id),
      share_experience_link_(share_experience_link),
      rule_name_(rule.Name()) {}

std::string PsstOperationContext::GetUserId() const {
  return user_id_;
}
std::string PsstOperationContext::GetRuleName() const {
  return rule_name_;
}

bool PsstOperationContext::IsValid() const {
  LOG(INFO) << "[PSST] IsValid #100 user_id_:" << user_id_;
  LOG(INFO) << "[PSST] IsValid result:"
            << (!user_id_.empty() && !share_experience_link_.empty() &&
                !rule_name_.empty());
  return !user_id_.empty() && !share_experience_link_.empty() &&
         !rule_name_.empty();
}

const std::optional<GURL> PsstOperationContext::GetShareLink(
    const std::u16string& pre_populate_text) const {
  if (share_experience_link_.empty()) {
    return std::nullopt;
  }

  url::RawCanonOutputT<char> buffer;
  url::EncodeURIComponent(base::UTF16ToUTF8(pre_populate_text), &buffer);
  std::string output(buffer.data(), buffer.length());

  return GURL(base::ReplaceStringPlaceholders(share_experience_link_, {output},
                                              nullptr));
}

}  // namespace psst
