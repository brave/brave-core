// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/web_state/ui/wk_web_view_shared_resources_provider.h"

#include <Foundation/Foundation.h>
#include <WebKit/WebKit.h>

#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "base/uuid.h"
#include "brave/ios/web/web_state/ui/wk_content_rule_list_registry.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/web_state/ui/wk_content_rule_list_util.h"

namespace web {

namespace {

// A key used to associate a WKWebViewSharedResourcesProvider with a
// BrowserState.
constexpr char kWKWebViewSharedResourcesProviderKeyName[] =
    "wk_web_view_shared_resources_provider";

// Converts `uuid` to an NSUUID.
NSUUID* ToNSUUID(const base::Uuid& uuid) {
  DCHECK(uuid.is_valid());
  // base::Uuid(...) uses lower-case but NSUUID uses upper-case, so convert
  // to upper-case before calling -initWithUUIDString: to avoid case issues.
  const std::string uuid_string = base::ToUpperASCII(uuid.AsLowercaseString());
  NSString* uuid_nsstring = base::SysUTF8ToNSString(uuid_string);
  NSUUID* nsuuid = [[NSUUID alloc] initWithUUIDString:uuid_nsstring];
  DCHECK(nsuuid);
  return nsuuid;
}

}  // namespace

// static
WKWebViewSharedResourcesProvider&
WKWebViewSharedResourcesProvider::FromBrowserState(
    BrowserState* browser_state) {
  CHECK(browser_state);
  if (!browser_state->GetUserData(kWKWebViewSharedResourcesProviderKeyName)) {
    browser_state->SetUserData(
        kWKWebViewSharedResourcesProviderKeyName,
        base::WrapUnique(new WKWebViewSharedResourcesProvider(browser_state)));
  }
  return *(static_cast<WKWebViewSharedResourcesProvider*>(
      browser_state->GetUserData(kWKWebViewSharedResourcesProviderKeyName)));
}

// static
void WKWebViewSharedResourcesProvider::DeleteDataStorageForIdentifier(
    const base::Uuid& uuid,
    base::OnceCallback<void(NSError*)> callback) {
  if (@available(iOS 17.0, *)) {
    // Calling either +removeDataStoreForIdentifier:completionHandler: or
    // +fetchAllDataStoreIdentifiers: crashes if no WKWebsiteDataStore
    // instance have been created in the app before.
    //
    // To prevent a crash, access the default data store. This is fine since
    // the default data store cannot be deleted, so there is no risk of
    // preventing the deletion that could happen if the code were trying to
    // load the store that it is supposed to delete.
    @autoreleasepool {
      std::ignore = [WKWebsiteDataStore defaultDataStore];
    }

    // The WebKit documentation does not specify on which queue the block
    // is run, so use base::BindPostTask(...) to ensure the callback will
    // be run on the calling sequence.
    auto completion = base::CallbackToBlock(base::BindPostTask(
        base::SequencedTaskRunner::GetCurrentDefault(), std::move(callback)));

    [WKWebsiteDataStore removeDataStoreForIdentifier:ToNSUUID(uuid)
                                   completionHandler:completion];
  } else {
    NOTREACHED();
  }
}

WKWebViewSharedResourcesProvider::WKWebViewSharedResourcesProvider(
    BrowserState* browser_state)
    : browser_state_(browser_state),
      content_rule_list_registry_(std::make_unique<WKContentRuleListRegistry>(
          browser_state->GetStatePath())) {
  Initialize();
}

WKWebViewSharedResourcesProvider::WKWebViewSharedResourcesProvider(
    BrowserState* browser_state,
    std::unique_ptr<WKContentRuleListRegistry> rule_list_registry)
    : browser_state_(browser_state),
      content_rule_list_registry_(std::move(rule_list_registry)) {
  CHECK_IS_TEST();
  Initialize();
}

WKWebViewSharedResourcesProvider::~WKWebViewSharedResourcesProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void WKWebViewSharedResourcesProvider::Initialize() {
  content_rule_list_registry_->UpdateRuleList(
      kBlockLocalResourcesRuleListKey,
      base::SysNSStringToUTF8(CreateAppSpecificURLsJsonRuleList()),
      base::DoNothing());
  content_rule_list_registry_->UpdateRuleList(
      kMixedContentUpgradeRuleListKey,
      base::SysNSStringToUTF8(CreateMixedContentAutoUpgradeJsonRuleList()),
      base::DoNothing());
}

base::WeakPtr<WKWebViewSharedResourcesProvider>
WKWebViewSharedResourcesProvider::AsWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return weak_ptr_factory_.GetWeakPtr();
}

WKWebsiteDataStore* WKWebViewSharedResourcesProvider::GetWebsiteDataStore() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (website_data_store_) {
    return website_data_store_;
  }

  TRACE_EVENT("ui", "WKWebViewSharedResourcesProvider::GetWebsiteDataStore");
  if (browser_state_->IsOffTheRecord()) {
    // The data is stored in memory. A new non-persistent data store is
    // created for each incognito browser state.
    website_data_store_ = [WKWebsiteDataStore nonPersistentDataStore];
  } else {
    const base::Uuid& storage_id = browser_state_->GetWebKitStorageID();
    if (storage_id.is_valid()) {
      if (@available(iOS 17.0, *)) {
        // `dataStoreForIdentifier:` is available after iOS 17. Otherwise, use
        // the default data store.
        website_data_store_ =
            [WKWebsiteDataStore dataStoreForIdentifier:ToNSUUID(storage_id)];
      }
    }
  }

  // Explicitly use the default data store.
  if (website_data_store_ == nil) {
    website_data_store_ = [WKWebsiteDataStore defaultDataStore];
  }

  // Workaround to force the creation of the WKWebsiteDataStore.
  NSSet* data_types = [NSSet setWithObject:WKWebsiteDataTypeCookies];
  [website_data_store_
      fetchDataRecordsOfTypes:data_types
            completionHandler:^(NSArray<WKWebsiteDataRecord*>* records){
            }];

  website_data_store_updated_callbacks_.Notify(website_data_store_);
  return website_data_store_;
}

void WKWebViewSharedResourcesProvider::SetExternalWebsiteDataStore(
    WKWebsiteDataStore* data_store) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(data_store);
  if (website_data_store_ == data_store) {
    return;
  }
  website_data_store_ = data_store;
  website_data_store_updated_callbacks_.Notify(website_data_store_);
}

void WKWebViewSharedResourcesProvider::SetExternalConfiguration(
    WKWebViewConfiguration* configuration) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(configuration);
  // Copied, so later changes to `configuration` don't apply to the WebStates
  // derived from it.
  external_configuration_ = [configuration copy];
}

WKWebViewConfiguration*
WKWebViewSharedResourcesProvider::GetExternalConfiguration() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return external_configuration_;
}

base::CallbackListSubscription
WKWebViewSharedResourcesProvider::RegisterWebSiteDataStoreUpdatedCallback(
    WebSiteDataStoreUpdatedCallbackList::CallbackType callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return website_data_store_updated_callbacks_.Add(std::move(callback));
}

WKContentRuleListRegistry&
WKWebViewSharedResourcesProvider::GetContentRuleListRegistry() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return *content_rule_list_registry_;
}

void WKWebViewSharedResourcesProvider::Purge() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

}  // namespace web
