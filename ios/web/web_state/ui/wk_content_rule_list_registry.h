// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_WEB_STATE_UI_WK_CONTENT_RULE_LIST_REGISTRY_H_
#define BRAVE_IOS_WEB_WEB_STATE_UI_WK_CONTENT_RULE_LIST_REGISTRY_H_

#include <Foundation/Foundation.h>

#include <map>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"

@class WKContentRuleList;
@class WKContentRuleListStore;

namespace base {
class FilePath;
}

namespace web {

// Compiles and stores the WKContentRuleLists of a BrowserState. Applying them
// to a WKUserContentController is left to WKContentRuleListApplier, so that the
// compiled lists can be shared by every WebState of the BrowserState.
//
// This class is not thread-safe and should only be accessed on the UI thread.
class WKContentRuleListRegistry {
 public:
  // A unique identifier for a content rule list.
  using RuleListKey = std::string;

  // Callback invoked after an asynchronous operation completes. `error` will
  // be nil on success, and non-nil if compilation or removal failed.
  using OperationCallback = base::OnceCallback<void(NSError* error)>;

  // Notified when the compiled rule lists change.
  class Observer : public base::CheckedObserver {
   public:
    // Called when the list for `key` was compiled. `previous_rule_list` is the
    // list it replaces, or nil if there was none.
    virtual void OnRuleListUpdated(const RuleListKey& key,
                                   WKContentRuleList* rule_list,
                                   WKContentRuleList* previous_rule_list) = 0;

    // Called when the list for `key` is removed.
    virtual void OnRuleListRemoved(const RuleListKey& key,
                                   WKContentRuleList* rule_list) = 0;
  };

  // `state_path` - The path where the BrowserState data is stored, in which to
  // store content rule lists.
  explicit WKContentRuleListRegistry(const base::FilePath& state_path);
  virtual ~WKContentRuleListRegistry();

  WKContentRuleListRegistry(const WKContentRuleListRegistry&) = delete;
  WKContentRuleListRegistry& operator=(const WKContentRuleListRegistry&) =
      delete;

  // Asynchronously creates or updates a content rule list identified by `key`.
  // The `callback` is invoked upon completion.
  virtual void UpdateRuleList(RuleListKey key,
                              std::string json_rules,
                              OperationCallback callback);

  // Asynchronously removes an existing content rule list identified by `key`.
  // The `callback` is invoked upon completion.
  virtual void RemoveRuleList(RuleListKey key, OperationCallback callback);

  // Returns all compiled content rule lists, keyed by their identifier.
  const std::map<RuleListKey, WKContentRuleList*>& GetCompiledRuleLists() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Returns a WeakPtr to the current instance.
  base::WeakPtr<WKContentRuleListRegistry> AsWeakPtr();

 private:
  // Callback invoked when a rule list is compiled by `rule_list_store_`.
  void OnRuleListCompiled(RuleListKey key,
                          OperationCallback callback,
                          base::TimeTicks start_time,
                          WKContentRuleList* rule_list,
                          NSError* error);

  // Callback invoked when a rule list is removed from `rule_list_store_`.
  void OnRuleListRemoved(RuleListKey key,
                         OperationCallback callback,
                         NSError* error);

  SEQUENCE_CHECKER(sequence_checker_);

  // A map of all compiled lists, keyed by their identifier.
  std::map<RuleListKey, WKContentRuleList*> compiled_lists_;
  // The persistent store where content rule lists are stored.
  WKContentRuleListStore* __strong rule_list_store_ = nil;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<WKContentRuleListRegistry> weak_ptr_factory_{this};
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_WEB_STATE_UI_WK_CONTENT_RULE_LIST_REGISTRY_H_
