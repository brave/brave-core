// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_SHARED_RESOURCES_PROVIDER_H_
#define BRAVE_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_SHARED_RESOURCES_PROVIDER_H_

#include <Foundation/Foundation.h>

#include <memory>

#include "base/callback_list.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/supports_user_data.h"

@class WKWebsiteDataStore;

namespace base {
class Uuid;
}

namespace web {

class BrowserState;
class WKContentRuleListRegistry;

// Keys for the static content rule lists managed by this provider.
inline constexpr char kBlockLocalResourcesRuleListKey[] = "BlockLocalResources";
inline constexpr char kMixedContentUpgradeRuleListKey[] = "MixedContentUpgrade";

// Owns the WKWebView resources shared by every WebState of a BrowserState: the
// WKWebsiteDataStore and the compiled WKContentRuleLists. Not thread safe. Must
// be used only on the main thread.
class WKWebViewSharedResourcesProvider : public base::SupportsUserData::Data {
 public:
  // Callbacks invoked when the website data store is updated.
  using WebSiteDataStoreUpdatedCallbackList =
      base::RepeatingCallbackList<void(WKWebsiteDataStore*)>;

  ~WKWebViewSharedResourcesProvider() override;

  WKWebViewSharedResourcesProvider(const WKWebViewSharedResourcesProvider&) =
      delete;
  WKWebViewSharedResourcesProvider& operator=(
      const WKWebViewSharedResourcesProvider&) = delete;

  // Returns the provider for `browser_state`, creating it if needed.
  // `browser_state` can not be null.
  static WKWebViewSharedResourcesProvider& FromBrowserState(
      BrowserState* browser_state);

  // Deletes the storage associated with `uuid`. This must only be called if no
  // storage is created for that identifier.
  static void DeleteDataStorageForIdentifier(
      const base::Uuid& uuid,
      base::OnceCallback<void(NSError*)> callback);

  // Returns a WeakPtr to the current instance.
  base::WeakPtr<WKWebViewSharedResourcesProvider> AsWeakPtr();

  // Returns the WKWebsiteDataStore of the BrowserState. Lazily creates the data
  // store if it does not exist, notifying the registered callbacks.
  WKWebsiteDataStore* GetWebsiteDataStore();

  // Replaces the data store with `data_store`, which comes from a
  // WKWebViewConfiguration not originating from //ios/web. `data_store` must
  // not be nil. Notifies the registered callbacks if the data store changes.
  void SetExternalWebsiteDataStore(WKWebsiteDataStore* data_store);

  // Drops the data store set by `SetExternalWebsiteDataStore`, if any, so that
  // the //ios/web data store of the BrowserState is used again.
  void ResetExternalWebsiteDataStore();

  // Registers `callback` to be invoked when the website data store is updated.
  base::CallbackListSubscription RegisterWebSiteDataStoreUpdatedCallback(
      WebSiteDataStoreUpdatedCallbackList::CallbackType callback);

  // Returns the registry of the BrowserState's compiled content rule lists.
  WKContentRuleListRegistry& GetContentRuleListRegistry();

  // Does nothing. WebKit lazily creates a configuration's visited link store
  // only once it is copied for a WKWebView, so each WKWebView has its own and
  // there is no shared state left to clear.
  void Purge();

 private:
  friend class WKWebViewSharedResourcesProviderTest;

  explicit WKWebViewSharedResourcesProvider(BrowserState* browser_state);
  // Allows injecting a custom rule list registry for testing.
  WKWebViewSharedResourcesProvider(
      BrowserState* browser_state,
      std::unique_ptr<WKContentRuleListRegistry> rule_list_registry);

  // Compiles the static content rule lists.
  void Initialize();

  SEQUENCE_CHECKER(sequence_checker_);

  raw_ptr<BrowserState> browser_state_;
  WKWebsiteDataStore* website_data_store_ = nil;

  // Whether `website_data_store_` originated from //ios/web, rather than a
  // WKWebViewConfiguration passed to //ios/web from outside of it.
  bool is_data_store_originated_from_ios_web_ = true;

  WebSiteDataStoreUpdatedCallbackList website_data_store_updated_callbacks_;

  std::unique_ptr<WKContentRuleListRegistry> content_rule_list_registry_;

  base::WeakPtrFactory<WKWebViewSharedResourcesProvider> weak_ptr_factory_{
      this};
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_SHARED_RESOURCES_PROVIDER_H_
