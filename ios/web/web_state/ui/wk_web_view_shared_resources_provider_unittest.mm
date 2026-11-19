// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/web_state/ui/wk_web_view_shared_resources_provider.h"

#include <WebKit/WebKit.h>

#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/uuid.h"
#include "brave/ios/web/web_state/ui/wk_content_rule_list_applier.h"
#include "brave/ios/web/web_state/ui/wk_content_rule_list_registry.h"
#include "ios/web/public/test/fakes/fake_browser_state.h"
#include "ios/web/public/test/fakes/fake_web_client.h"
#include "ios/web/public/test/scoped_testing_web_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

namespace web {

namespace {

// A valid, but minimal, content rule list for testing.
constexpr char kValidTestRuleListJson[] = R"([
  {
    "trigger": {"url-filter": ".*"},
    "action": {"type": "block"}
  }
])";

class MockWKContentRuleListRegistry : public WKContentRuleListRegistry {
 public:
  explicit MockWKContentRuleListRegistry(const base::FilePath& path)
      : WKContentRuleListRegistry(path) {}
  ~MockWKContentRuleListRegistry() override = default;

  MOCK_METHOD(void,
              UpdateRuleList,
              (RuleListKey key,
               std::string json_rules,
               OperationCallback callback),
              (override));
  MOCK_METHOD(void,
              RemoveRuleList,
              (RuleListKey key, OperationCallback callback),
              (override));
};

}  // namespace

class WKWebViewSharedResourcesProviderTest : public PlatformTest {
 protected:
  // Creates a provider with `rule_list_registry`. This can call the private
  // constructor because WKWebViewSharedResourcesProvider declares this test
  // class as a friend.
  std::unique_ptr<WKWebViewSharedResourcesProvider>
  CreateProviderWithRuleListRegistry(
      BrowserState* browser_state,
      std::unique_ptr<WKContentRuleListRegistry> rule_list_registry) {
    return base::WrapUnique(new WKWebViewSharedResourcesProvider(
        browser_state, std::move(rule_list_registry)));
  }

  WKWebViewSharedResourcesProvider& GetProvider(BrowserState* browser_state) {
    return WKWebViewSharedResourcesProvider::FromBrowserState(browser_state);
  }

  // Compiles `json_rules` for `key` on `registry`, returning the error.
  NSError* UpdateRuleListSync(WKContentRuleListRegistry& registry,
                              const std::string& key,
                              const std::string& json_rules) {
    base::test::TestFuture<NSError*> future;
    registry.UpdateRuleList(key, json_rules, future.GetCallback());
    return future.Get();
  }

  // Removes the list for `key` from `registry`, returning the error.
  NSError* RemoveRuleListSync(WKContentRuleListRegistry& registry,
                              const std::string& key) {
    base::test::TestFuture<NSError*> future;
    registry.RemoveRuleList(key, future.GetCallback());
    return future.Get();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::UI};
  ScopedTestingWebClient web_client_{std::make_unique<FakeWebClient>()};
};

// Tests that the static content rule lists are compiled on creation.
TEST_F(WKWebViewSharedResourcesProviderTest, StaticContentRuleListsAreCreated) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/true);
  auto rule_list_registry =
      std::make_unique<testing::StrictMock<MockWKContentRuleListRegistry>>(
          browser_state.GetStatePath());
  EXPECT_CALL(
      *rule_list_registry,
      UpdateRuleList(kBlockLocalResourcesRuleListKey, testing::_, testing::_));
  EXPECT_CALL(
      *rule_list_registry,
      UpdateRuleList(kMixedContentUpgradeRuleListKey, testing::_, testing::_));

  auto provider = CreateProviderWithRuleListRegistry(
      &browser_state, std::move(rule_list_registry));
}

// Tests that the same provider is returned for a BrowserState.
TEST_F(WKWebViewSharedResourcesProviderTest, SameProviderForBrowserState) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/true);
  EXPECT_EQ(&GetProvider(&browser_state), &GetProvider(&browser_state));
}

// Tests that the data store is persistent for a BrowserState that is not off
// the record, and the same one is returned every time.
TEST_F(WKWebViewSharedResourcesProviderTest, GetWebsiteDataStore) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/true);
  WKWebsiteDataStore* data_store =
      GetProvider(&browser_state).GetWebsiteDataStore();
  ASSERT_TRUE(data_store);
  EXPECT_TRUE(data_store.persistent);
  EXPECT_NSEQ(data_store, GetProvider(&browser_state).GetWebsiteDataStore());
}

// Tests that the data store is not persistent for an off the record
// BrowserState.
TEST_F(WKWebViewSharedResourcesProviderTest, GetWebsiteDataStore_OffTheRecord) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/true);
  browser_state.SetOffTheRecord(true);
  WKWebsiteDataStore* data_store =
      GetProvider(&browser_state).GetWebsiteDataStore();
  ASSERT_TRUE(data_store);
  EXPECT_FALSE(data_store.persistent);
}

// Tests that the default data store is used without a storage ID.
TEST_F(WKWebViewSharedResourcesProviderTest, GetWebsiteDataStore_Default) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/false);
  EXPECT_NSEQ(WKWebsiteDataStore.defaultDataStore,
              GetProvider(&browser_state).GetWebsiteDataStore());
}

// Tests that BrowserStates with the same storage ID share a data store, and
// those with a different one do not.
TEST_F(WKWebViewSharedResourcesProviderTest, GetWebsiteDataStore_StorageID) {
  const base::Uuid uuid = base::Uuid::GenerateRandomV4();
  FakeBrowserState browser_state1(/*use_unique_storage_uuid=*/false);
  browser_state1.SetWebKitStorageID(uuid);
  FakeBrowserState browser_state2(/*use_unique_storage_uuid=*/false);
  browser_state2.SetWebKitStorageID(uuid);
  FakeBrowserState browser_state3(/*use_unique_storage_uuid=*/true);

  WKWebsiteDataStore* data_store1 =
      GetProvider(&browser_state1).GetWebsiteDataStore();
  EXPECT_NSEQ(data_store1, GetProvider(&browser_state2).GetWebsiteDataStore());
  EXPECT_NSNE(data_store1, GetProvider(&browser_state3).GetWebsiteDataStore());
}

// Tests that the data store callbacks are notified when the data store is
// created or replaced, and not when it is unchanged.
TEST_F(WKWebViewSharedResourcesProviderTest, WebSiteDataStoreUpdatedCallbacks) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/false);
  WKWebViewSharedResourcesProvider& provider = GetProvider(&browser_state);

  __block WKWebsiteDataStore* recorded_data_store = nil;
  __block int notification_count = 0;
  base::CallbackListSubscription subscription =
      provider.RegisterWebSiteDataStoreUpdatedCallback(
          base::BindRepeating(^(WKWebsiteDataStore* data_store) {
            recorded_data_store = data_store;
            ++notification_count;
          }));
  ASSERT_EQ(0, notification_count);

  // Creating the data store notifies.
  WKWebsiteDataStore* data_store = provider.GetWebsiteDataStore();
  EXPECT_NSEQ(data_store, recorded_data_store);
  EXPECT_EQ(1, notification_count);

  // Getting it again does not.
  provider.GetWebsiteDataStore();
  EXPECT_EQ(1, notification_count);

  // Resetting the external data store without one set does nothing.
  provider.ResetExternalWebsiteDataStore();
  EXPECT_NSEQ(data_store, provider.GetWebsiteDataStore());
  EXPECT_EQ(1, notification_count);

  // An external data store replaces it.
  WKWebsiteDataStore* external_data_store =
      WKWebsiteDataStore.nonPersistentDataStore;
  provider.SetExternalWebsiteDataStore(external_data_store);
  EXPECT_NSEQ(external_data_store, recorded_data_store);
  EXPECT_NSEQ(external_data_store, provider.GetWebsiteDataStore());
  EXPECT_EQ(2, notification_count);

  // Setting the same one again does not notify.
  provider.SetExternalWebsiteDataStore(external_data_store);
  EXPECT_EQ(2, notification_count);

  // Resetting it restores the BrowserState's own data store.
  provider.ResetExternalWebsiteDataStore();
  EXPECT_NSEQ(data_store, provider.GetWebsiteDataStore());
  EXPECT_NSEQ(data_store, recorded_data_store);
  EXPECT_EQ(3, notification_count);
}

// Tests that an applier installs the compiled lists on its controller, and
// keeps them in sync as they are updated and removed.
TEST_F(WKWebViewSharedResourcesProviderTest, ContentRuleListApplier) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/true);
  WKContentRuleListRegistry registry(browser_state.GetStatePath());
  const std::string key = "test_key";
  ASSERT_FALSE(UpdateRuleListSync(registry, key, kValidTestRuleListJson));
  WKContentRuleList* rule_list = registry.GetCompiledRuleLists().at(key);
  ASSERT_TRUE(rule_list);

  // Mocking the controllers records the lists installed on them, as
  // WKUserContentController does not expose them.
  id controller1 = OCMClassMock([WKUserContentController class]);
  id controller2 = OCMClassMock([WKUserContentController class]);

  // Setting a controller installs the already compiled lists.
  WKContentRuleListApplier applier1(registry);
  OCMExpect([controller1 addContentRuleList:rule_list]);
  applier1.SetUserContentController(controller1);
  EXPECT_OCMOCK_VERIFY(controller1);

  WKContentRuleListApplier applier2(registry);
  OCMExpect([controller2 addContentRuleList:rule_list]);
  applier2.SetUserContentController(controller2);
  EXPECT_OCMOCK_VERIFY(controller2);

  // Updating a list replaces it on every controller.
  OCMExpect([controller1 removeContentRuleList:rule_list]);
  OCMExpect([controller1 addContentRuleList:[OCMArg any]]);
  OCMExpect([controller2 removeContentRuleList:rule_list]);
  OCMExpect([controller2 addContentRuleList:[OCMArg any]]);
  ASSERT_FALSE(UpdateRuleListSync(registry, key, kValidTestRuleListJson));
  EXPECT_OCMOCK_VERIFY(controller1);
  EXPECT_OCMOCK_VERIFY(controller2);
  WKContentRuleList* updated_rule_list =
      registry.GetCompiledRuleLists().at(key);

  // Removing a list removes it from every controller.
  OCMExpect([controller1 removeContentRuleList:updated_rule_list]);
  OCMExpect([controller2 removeContentRuleList:updated_rule_list]);
  ASSERT_FALSE(RemoveRuleListSync(registry, key));
  EXPECT_OCMOCK_VERIFY(controller1);
  EXPECT_OCMOCK_VERIFY(controller2);
  EXPECT_TRUE(registry.GetCompiledRuleLists().empty());
}

// Tests that moving an applier to a new controller uninstalls the lists from
// the previous one.
TEST_F(WKWebViewSharedResourcesProviderTest,
       ContentRuleListApplier_ChangeController) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/true);
  WKContentRuleListRegistry registry(browser_state.GetStatePath());
  const std::string key = "test_key";
  ASSERT_FALSE(UpdateRuleListSync(registry, key, kValidTestRuleListJson));
  WKContentRuleList* rule_list = registry.GetCompiledRuleLists().at(key);

  id controller1 = OCMClassMock([WKUserContentController class]);
  id controller2 = OCMClassMock([WKUserContentController class]);

  WKContentRuleListApplier applier(registry);
  OCMExpect([controller1 addContentRuleList:rule_list]);
  applier.SetUserContentController(controller1);
  EXPECT_OCMOCK_VERIFY(controller1);

  OCMExpect([controller1 removeContentRuleList:rule_list]);
  OCMExpect([controller2 addContentRuleList:rule_list]);
  applier.SetUserContentController(controller2);
  EXPECT_OCMOCK_VERIFY(controller1);
  EXPECT_OCMOCK_VERIFY(controller2);
}

// Tests that an applier outliving its registry does not crash.
TEST_F(WKWebViewSharedResourcesProviderTest,
       ContentRuleListApplier_OutlivesRegistry) {
  FakeBrowserState browser_state(/*use_unique_storage_uuid=*/true);
  auto registry =
      std::make_unique<WKContentRuleListRegistry>(browser_state.GetStatePath());
  WKContentRuleListApplier applier(*registry);
  registry.reset();
  applier.SetUserContentController([[WKUserContentController alloc] init]);
}

}  // namespace web
