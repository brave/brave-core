// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/web_state/ui/wk_content_rule_list_registry.h"

#include <WebKit/WebKit.h>

#include "base/apple/foundation_util.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/sys_string_conversions.h"

namespace web {

WKContentRuleListRegistry::WKContentRuleListRegistry(
    const base::FilePath& state_path) {
  rule_list_store_ = [WKContentRuleListStore
      storeWithURL:base::apple::FilePathToNSURL(state_path)];
  CHECK(rule_list_store_);
}

WKContentRuleListRegistry::~WKContentRuleListRegistry() = default;

void WKContentRuleListRegistry::UpdateRuleList(RuleListKey key,
                                               std::string json_rules,
                                               OperationCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  NSString* identifier = base::SysUTF8ToNSString(key);
  NSString* rules = base::SysUTF8ToNSString(json_rules);

  void (^completion_handler)(WKContentRuleList*, NSError*) =
      base::CallbackToBlock(
          base::BindOnce(&WKContentRuleListRegistry::OnRuleListCompiled,
                         weak_ptr_factory_.GetWeakPtr(), key,
                         std::move(callback), base::TimeTicks::Now()));

  [rule_list_store_ compileContentRuleListForIdentifier:identifier
                                 encodedContentRuleList:rules
                                      completionHandler:completion_handler];
}

void WKContentRuleListRegistry::RemoveRuleList(RuleListKey key,
                                               OperationCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = compiled_lists_.find(key);
  if (it == compiled_lists_.end()) {
    // If the list is not tracked, assume it's not in the store either.
    // Just run the callback with success.
    std::move(callback).Run(nil);
    return;
  }
  // Notify observers first so they immediately stop applying the list.
  for (Observer& observer : observers_) {
    observer.OnRuleListRemoved(key, it->second);
  }

  // Now, asynchronously remove it from the persistent WKContentRuleListStore.
  NSString* identifier = base::SysUTF8ToNSString(key);
  [rule_list_store_
      removeContentRuleListForIdentifier:identifier
                       completionHandler:
                           base::CallbackToBlock(base::BindOnce(
                               &WKContentRuleListRegistry::OnRuleListRemoved,
                               weak_ptr_factory_.GetWeakPtr(), key,
                               std::move(callback)))];
}

const std::map<WKContentRuleListRegistry::RuleListKey, WKContentRuleList*>&
WKContentRuleListRegistry::GetCompiledRuleLists() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return compiled_lists_;
}

void WKContentRuleListRegistry::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void WKContentRuleListRegistry::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

base::WeakPtr<WKContentRuleListRegistry>
WKContentRuleListRegistry::AsWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return weak_ptr_factory_.GetWeakPtr();
}

void WKContentRuleListRegistry::OnRuleListCompiled(RuleListKey key,
                                                   OperationCallback callback,
                                                   base::TimeTicks start_time,
                                                   WKContentRuleList* rule_list,
                                                   NSError* error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const base::TimeDelta duration = base::TimeTicks::Now() - start_time;
  // This case is not expected. WebKit should return either a list or an
  // error.
  if (!rule_list && !error) {
    error = [NSError
        errorWithDomain:WKErrorDomain
                   code:WKErrorUnknown
               userInfo:@{
                 NSLocalizedDescriptionKey :
                     @"Rule list compilation returned no list and no error."
               }];
  }

  base::UmaHistogramBoolean(
      "IOS.ContentRuleListProvider.Compile.Success." + key, error == nil);

  if (error) {
    base::UmaHistogramSparse("IOS.ContentRuleListProvider.Compile.Error." + key,
                             error.code);
  } else {
    base::UmaHistogramTimes("IOS.ContentRuleListProvider.Compile.Time." + key,
                            duration);

    WKContentRuleList* previous_rule_list = nil;
    auto it = compiled_lists_.find(key);
    if (it != compiled_lists_.end()) {
      previous_rule_list = it->second;
    }
    compiled_lists_[key] = rule_list;

    for (Observer& observer : observers_) {
      observer.OnRuleListUpdated(key, rule_list, previous_rule_list);
    }
  }

  // Notify the original caller of the result.
  std::move(callback).Run(error);
}

void WKContentRuleListRegistry::OnRuleListRemoved(RuleListKey key,
                                                  OperationCallback callback,
                                                  NSError* error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // A "not found" error is not a failure for a removal operation. The goal is
  // to ensure the list is gone from the store.
  if (error && ([error.domain isEqualToString:WKErrorDomain] &&
                error.code == WKErrorContentRuleListStoreLookUpFailed)) {
    error = nil;
  }

  base::UmaHistogramBoolean("IOS.ContentRuleListProvider.Remove.Success." + key,
                            error == nil);

  // Only remove from the internal map on success.
  if (!error) {
    compiled_lists_.erase(key);
  }
  std::move(callback).Run(error);
}

}  // namespace web
