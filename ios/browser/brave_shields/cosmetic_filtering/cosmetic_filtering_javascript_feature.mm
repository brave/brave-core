// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_javascript_feature.h"

#import <Foundation/Foundation.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_reader.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "brave/ios/browser/api/url/url_utils.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_args.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "cosmetic_filtering";
constexpr char kMessageRequestTypeKey[] = "request_type";
constexpr char kMessageRequestTypeArgsKey[] = "args";
constexpr char kMessageRequestTypeSelectorsKey[] = "selectors";
constexpr char kMessageRequestTypePartinessKey[] = "url_partiness";
constexpr char kMessageDataKey[] = "data";
constexpr char kMessageIdsTypeKey[] = "ids";
constexpr char kMessageClassesTypeKey[] = "classes";
constexpr char kMessageURLsKey[] = "urls";

// Extract a list of strings stored at `key` in `dict`. Non-string entries are
// skipped. Returns an empty vector when the key is missing or not a list.
std::vector<std::string> StringListFrom(const base::DictValue& dict,
                                        std::string_view key) {
  std::vector<std::string> result;
  const base::ListValue* list = dict.FindList(key);
  if (!list) {
    return result;
  }
  result.reserve(list->size());
  for (const base::Value& entry : *list) {
    if (const std::string* value = entry.GetIfString()) {
      result.push_back(*value);
    }
  }
  return result;
}

// Build the script `args` response from `args`, mirroring the `Arguments`
// type in cosmetic_filtering.ts. `proceduralFilters` are stored as raw JSON
// strings and are parsed back into objects; entries that fail to parse are
// skipped.
base::Value ArgsResponseDictFrom(CosmeticFilteringArgs* args) {
  base::DictValue response;
  response.Set("hideFirstPartyContent",
               static_cast<bool>(args.hideFirstPartyContent));
  response.Set("genericHide", static_cast<bool>(args.genericHide));
  if (args.firstSelectorsPollingDelayMs) {
    response.Set("firstSelectorsPollingDelayMs",
                 args.firstSelectorsPollingDelayMs.intValue);
  }
  if (args.switchToSelectorsPollingThreshold) {
    response.Set("switchToSelectorsPollingThreshold",
                 args.switchToSelectorsPollingThreshold.intValue);
  }
  if (args.fetchNewClassIdRulesThrottlingMs) {
    response.Set("fetchNewClassIdRulesThrottlingMs",
                 args.fetchNewClassIdRulesThrottlingMs.intValue);
  }

  base::ListValue standard_list;
  for (NSString* selector in args.standardSelectors) {
    standard_list.Append(base::SysNSStringToUTF8(selector));
  }
  response.Set("standardSelectors", std::move(standard_list));

  base::ListValue aggressive_list;
  for (NSString* selector in args.aggressiveSelectors) {
    aggressive_list.Append(base::SysNSStringToUTF8(selector));
  }
  response.Set("aggressiveSelectors", std::move(aggressive_list));

  base::ListValue procedural_list;
  for (NSString* filter in args.proceduralFilters) {
    std::optional<base::Value> parsed = base::JSONReader::Read(
        base::SysNSStringToUTF8(filter), base::JSON_PARSE_RFC);
    if (parsed) {
      procedural_list.Append(std::move(*parsed));
    }
  }
  response.Set("proceduralFilters", std::move(procedural_list));

  return base::Value(std::move(response));
}

// Build `{'standardSelectors': [], 'aggressiveSelectors': []}` response,
// mirroring the result type of `sendSelectors()` in cosmetic_filtering.ts.
base::Value SelectorsResponseDictFrom(
    std::vector<std::string> standard_selectors,
    std::vector<std::string> aggressive_selectors) {
  base::ListValue standard_list;
  for (auto& selector : standard_selectors) {
    standard_list.Append(std::move(selector));
  }
  base::ListValue aggressive_list;
  for (auto& selector : aggressive_selectors) {
    aggressive_list.Append(std::move(selector));
  }
  base::DictValue response;
  response.Set("standardSelectors", std::move(standard_list));
  response.Set("aggressiveSelectors", std::move(aggressive_list));
  return base::Value(std::move(response));
}

// Build a `{url: is_first_party}` response mapping each url in `urls` to
// whether it shares the same eTLD+1 as `frame_url`. Non-string entries are
// skipped.
base::Value PartinessResponseDictFrom(const base::ListValue& urls,
                                      const GURL& frame_url) {
  NSString* frame_etld = net::NSURLWithGURL(frame_url).brave_domainAndRegistry;
  base::DictValue response;
  for (const base::Value& entry : urls) {
    const std::string* url_string = entry.GetIfString();
    if (!url_string) {
      continue;
    }
    NSString* url_etld =
        net::NSURLWithGURL(GURL(*url_string)).brave_domainAndRegistry;
    response.Set(*url_string, [frame_etld isEqualToString:url_etld]);
  }
  return base::Value(std::move(response));
}

}  // namespace

CosmeticFilteringJavaScriptFeature::CosmeticFilteringJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kAllFrames,
              FeatureScript::ReinjectionBehavior::kReinjectOnDocumentRecreation,
              base::BindRepeating(
                  &CosmeticFilteringJavaScriptFeature::GetReplacements,
                  base::Unretained(this)))}) {}

CosmeticFilteringJavaScriptFeature::~CosmeticFilteringJavaScriptFeature() =
    default;

// static
CosmeticFilteringJavaScriptFeature*
CosmeticFilteringJavaScriptFeature::GetInstance() {
  static base::NoDestructor<CosmeticFilteringJavaScriptFeature> instance;
  return instance.get();
}

web::JavaScriptFeature::FeatureScript::PlaceholderReplacements
CosmeticFilteringJavaScriptFeature::GetReplacements() {
  NSMutableDictionary* replacements = [[NSMutableDictionary alloc] init];
  [replacements addEntriesFromDictionary:token_.GetPlaceholderReplacements()];
  [replacements
      addEntriesFromDictionary:handler_name_.GetPlaceholderReplacements()];
  return [replacements copy];
}

std::optional<std::string>
CosmeticFilteringJavaScriptFeature::GetScriptMessageHandlerName() const {
  return handler_name_.GetScriptMessageHandlerName();
}

bool CosmeticFilteringJavaScriptFeature::GetFeatureRepliesToPrompts() const {
  return true;
}

bool CosmeticFilteringJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

void CosmeticFilteringJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  auto body = token_.GetValidatedScriptMessageBody(message);
  const base::DictValue* script_dict =
      body ? body.value()->GetIfDict() : nullptr;
  if (!script_dict) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const std::string* request_type_string =
      script_dict->FindString(kMessageRequestTypeKey);
  if (!request_type_string) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const GURL frame_url = message.security_origin().GetURL();
  if (!frame_url.is_valid()) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  if (*request_type_string == kMessageRequestTypeArgsKey) {
    auto* tab_helper = CosmeticFilteringTabHelper::FromWebState(web_state);
    if (!tab_helper) {
      std::move(callback).Run(nullptr, nil);
      return;
    }

    // Ask the tab helper for the cosmetic filtering args for this frame, then
    // reply with the script `args` object.
    tab_helper->CosmeticFilteringArgsFor(
        frame_url, base::BindOnce(
                       [](ScriptMessageReplyCallback callback,
                          CosmeticFilteringArgs* args) {
                         if (!args) {
                           std::move(callback).Run(nullptr, nil);
                           return;
                         }
                         base::Value response = ArgsResponseDictFrom(args);
                         std::move(callback).Run(&response, nil);
                       },
                       std::move(callback)));
    return;
  } else if (*request_type_string == kMessageRequestTypeSelectorsKey) {
    const base::DictValue* data_dict = script_dict->FindDict(kMessageDataKey);
    auto* tab_helper = CosmeticFilteringTabHelper::FromWebState(web_state);
    if (!data_dict || !tab_helper) {
      std::move(callback).Run(nullptr, nil);
      return;
    }

    std::vector<std::string> ids =
        StringListFrom(*data_dict, kMessageIdsTypeKey);
    std::vector<std::string> classes =
        StringListFrom(*data_dict, kMessageClassesTypeKey);

    // Ask the tab helper which of the frames's ids/classes should be hidden,
    // then reply with the {standard, aggressive} selectors to hide.
    tab_helper->SelectorsToHideFor(
        frame_url, ids, classes,
        base::BindOnce(
            [](ScriptMessageReplyCallback callback,
               const std::vector<std::string>* standard_selectors,
               const std::vector<std::string>* aggressive_selectors) {
              if (!standard_selectors || !aggressive_selectors) {
                std::move(callback).Run(nullptr, nil);
                return;
              }
              base::Value response = SelectorsResponseDictFrom(
                  *standard_selectors, *aggressive_selectors);
              std::move(callback).Run(&response, nil);
            },
            std::move(callback)));
    return;
  } else if (*request_type_string == kMessageRequestTypePartinessKey) {
    const base::DictValue* data_dict = script_dict->FindDict(kMessageDataKey);
    if (!data_dict) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    const base::ListValue* urls = data_dict->FindList(kMessageURLsKey);
    if (!urls) {
      std::move(callback).Run(nullptr, nil);
      return;
    }

    base::Value response = PartinessResponseDictFrom(*urls, frame_url);
    std::move(callback).Run(&response, nil);
    return;
  }
  std::move(callback).Run(nullptr, nil);
}
