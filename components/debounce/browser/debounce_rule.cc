/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/debounce/browser/debounce_rule.h"

#include <memory>
#include <utility>

#include "base/base64url.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {
// debounce.json keys
const char kInclude[] = "include";
const char kExclude[] = "exclude";
const char kAction[] = "action";
const char kParam[] = "param";
}  // namespace

namespace debounce {

DebounceRule::DebounceRule() : action_(kDebounceNoAction) {}

DebounceRule::~DebounceRule() = default;

// static
bool DebounceRule::ParseDebounceAction(base::StringPiece value,
                                       DebounceAction* field) {
  if (value == "redirect") {
    *field = kDebounceRedirectToParam;
  } else if (value == "base64,redirect") {
    *field = kDebounceBase64DecodeAndRedirectToParam;
  } else {
    LOG(INFO) << "Found unknown debouncing action: " << value;
    return false;
  }
  return true;
}

// static
bool DebounceRule::GetURLPatternSetFromValue(
    const base::Value* value,
    extensions::URLPatternSet* result) {
  if (!value->is_list())
    return false;
  // Debouncing only affects HTTP or HTTPS URLs, regardless of how the rules are
  // written. (Also, don't write rules for other URL schemes, because they won't
  // work and you're just wasting everyone's time.)
  std::string error;
  bool valid = result->Populate(
      base::Value::AsListValue(*value),
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, false, &error);
  if (!valid)
    LOG(WARNING) << error;
  return valid;
}

// static
void DebounceRule::RegisterJSONConverter(
    base::JSONValueConverter<DebounceRule>* converter) {
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kInclude, &DebounceRule::include_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomValueField<extensions::URLPatternSet>(
      kExclude, &DebounceRule::exclude_pattern_set_, GetURLPatternSetFromValue);
  converter->RegisterCustomField<DebounceAction>(
      kAction, &DebounceRule::action_, &ParseDebounceAction);
  converter->RegisterStringField(kParam, &DebounceRule::param_);
}

bool DebounceRule::Apply(const GURL& original_url, GURL* final_url) const {
  // Unknown actions always return false, to allow for future updates to the
  // rules file which may be pushed to users before a new version of the code
  // that parses it.
  if (action_ != kDebounceRedirectToParam &&
      action_ != kDebounceBase64DecodeAndRedirectToParam)
    return false;
  // If URL matches an explicitly excluded pattern, this rule does not apply.
  if (exclude_pattern_set_.MatchesURL(original_url))
    return false;
  // If URL does not match an explicitly included pattern, this rule does not
  // apply.
  if (!include_pattern_set_.MatchesURL(original_url))
    return false;

  std::string unescaped_value;
  if (!net::GetValueForKeyInQuery(original_url, param_, &unescaped_value))
    return false;
  if ((action_ == kDebounceBase64DecodeAndRedirectToParam) &&
      (!base::Base64UrlDecode(unescaped_value,
                              base::Base64UrlDecodePolicy::IGNORE_PADDING,
                              &unescaped_value))) {
    return false;
  }
  GURL new_url(unescaped_value);

  // Failsafe: ensure we got a valid URL out of the param.
  if (!new_url.is_valid() || !new_url.SchemeIsHTTPOrHTTPS())
    return false;

  // Failsafe: never redirect to the same site.
  if (url::IsSameOriginWith(original_url, new_url))
    return false;

  *final_url = new_url;
  return true;
}

}  // namespace debounce
