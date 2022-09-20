/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"

#include <memory>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "extensions/common/url_pattern.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave {

namespace {
bool CreateURLPatternSetFromValue(const base::Value* value,
                                  extensions::URLPatternSet* result) {
  if (!value || !value->is_list())
    return false;
  std::string error;
  bool valid = result->Populate(
      value->GetList(), URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      false, &error);
  if (!valid) {
    VLOG(1) << "Unable to create url pattern:" << error;
  }
  return valid;
}

absl::optional<base::flat_set<std::string>> CreateParamsList(
    const base::Value* value) {
  if (!value->is_list())
    return absl::nullopt;
  base::flat_set<std::string> result;
  for (const auto& param : value->GetList()) {
    DCHECK(param.is_string());
    result.insert(param.GetString());
  }
  return result;
}

base::flat_set<std::unique_ptr<URLSanitizerService::MatchItem>> ParseFromJson(
    const std::string& json) {
  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(json);
  if (!parsed_json.has_value()) {
    VLOG(1) << "Error parsing feature JSON: " << parsed_json.error().message;
    return {};
  }
  const base::Value::List* list = parsed_json->GetIfList();
  if (!list) {
    return {};
  }
  base::flat_set<std::unique_ptr<URLSanitizerService::MatchItem>> matchers;
  for (const auto& it : *list) {
    const base::Value::Dict* items = it.GetIfDict();
    if (!items)
      continue;
    auto* include_list = items->Find("include");
    if (!include_list)
      continue;
    extensions::URLPatternSet include_matcher;
    if (!CreateURLPatternSetFromValue(include_list, &include_matcher))
      continue;
    auto* params_list = items->Find("params");
    absl::optional<base::flat_set<std::string>> params =
        CreateParamsList(params_list);
    if (!params) {
      continue;
    }

    extensions::URLPatternSet exclude_matcher;
    CreateURLPatternSetFromValue(it.FindListPath("exclude"), &exclude_matcher);
    auto item = std::make_unique<URLSanitizerService::MatchItem>(
        std::move(include_matcher), std::move(exclude_matcher),
        std::move(*params));

    matchers.insert(std::move(item));
  }

  return matchers;
}

}  // namespace

URLSanitizerService::URLSanitizerService() = default;

URLSanitizerService::~URLSanitizerService() = default;

URLSanitizerService::MatchItem::MatchItem() = default;
URLSanitizerService::MatchItem::~MatchItem() = default;

URLSanitizerService::MatchItem::MatchItem(extensions::URLPatternSet in,
                                          extensions::URLPatternSet ex,
                                          base::flat_set<std::string> prm)
    : include(std::move(in)), exclude(std::move(ex)), params(std::move(prm)) {}

void URLSanitizerService::Initialize(const std::string& json) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(&ParseFromJson, json),
      base::BindOnce(&URLSanitizerService::UpdateMatchers,
                     weak_factory_.GetWeakPtr()));
}

void URLSanitizerService::UpdateMatchers(
    base::flat_set<std::unique_ptr<URLSanitizerService::MatchItem>> mappings) {
  matchers_ = std::move(mappings);
  if (initialization_callback_for_testing_)
    std::move(initialization_callback_for_testing_).Run();
}

GURL URLSanitizerService::SanitizeURL(const GURL& initial_url) {
  if (matchers_.empty())
    return initial_url;
  GURL url = initial_url;
  for (const auto& it : matchers_) {
    if (!it->include.MatchesURL(url) || it->exclude.MatchesURL(url))
      continue;
    auto sanitized_query = StripQueryParameter(url.query(), it->params);
    GURL::Replacements replacements;
    if (!sanitized_query.empty()) {
      replacements.SetQueryStr(sanitized_query);
    } else {
      replacements.ClearQuery();
    }
    url = url.ReplaceComponents(replacements);
  }
  return url;
}

void URLSanitizerService::OnRulesReady(const std::string& json_content) {
  Initialize(json_content);
}

// FIXME: merge with
// browser/net/brave_site_hacks_network_delegate_helper.cc::StripQueryParameter()
// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
std::string URLSanitizerService::StripQueryParameter(
    const std::string& query,
    const base::flat_set<std::string>& trackers) {
  // We are using custom query string parsing code here. See
  // https://github.com/brave/brave-core/pull/13726#discussion_r897712350
  // for more information on why this approach was selected.
  //
  // Split query string by ampersands, remove tracking parameters,
  // then join the remaining query parameters, untouched, back into
  // a single query string.
  const std::vector<std::string> input_kv_strings =
      SplitString(query, "&", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  std::vector<std::string> output_kv_strings;
  int disallowed_count = 0;
  for (const std::string& kv_string : input_kv_strings) {
    const std::vector<std::string> pieces = SplitString(
        kv_string, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    const std::string& key = pieces.empty() ? std::string() : pieces[0];
    if (pieces.size() >= 2 && trackers.count(key) == 1) {
      ++disallowed_count;
    } else {
      output_kv_strings.push_back(kv_string);
    }
  }
  if (disallowed_count > 0) {
    return base::JoinString(output_kv_strings, "&");
  } else {
    return query;
  }
}

}  // namespace brave
