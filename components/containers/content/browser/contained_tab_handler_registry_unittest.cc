// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/contained_tab_handler_registry.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/gtest_util.h"
#include "brave/components/containers/content/browser/contained_tab_handler.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace containers {

namespace {

// Mock ContainedTabHandler for testing
class MockContainedTabHandler : public ContainedTabHandler {
 public:
  explicit MockContainedTabHandler(std::string id) : id_(std::move(id)) {}
  ~MockContainedTabHandler() override = default;

  const std::string& GetId() const override { return id_; }

 private:
  std::string id_;
};

}  // namespace

class ContainedTabHandlerRegistryTest : public testing::Test {
 public:
  ContainedTabHandlerRegistryTest() = default;
  ~ContainedTabHandlerRegistryTest() override = default;

  void SetUp() override {
    browser_context_ = std::make_unique<content::TestBrowserContext>();
  }

  void TearDown() override { browser_context_.reset(); }

 protected:
  ContainedTabHandlerRegistry& GetRegistry() {
    return ContainedTabHandlerRegistry::GetInstance();
  }

  std::unique_ptr<MockContainedTabHandler> CreateMockHandler(
      const std::string& name) {
    return std::make_unique<MockContainedTabHandler>(
        std::string(ContainedTabHandler::kIdPrefix) + name);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
};

// Test handler registration
TEST_F(ContainedTabHandlerRegistryTest, RegisterContainedTabHandler_Success) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("test-handler");
  const std::string handler_id = handler->GetId();

  registry.RegisterContainedTabHandler(std::move(handler));

  // Verify handler was registered by checking if its partition domain is
  // recognized
  content::StoragePartitionConfig config =
      content::StoragePartitionConfig::Create(
          browser_context_.get(), handler_id, "partition-name", false);

  EXPECT_TRUE(registry.ShouldInheritStoragePartition(config));
}

// Test ShouldInheritStoragePartition with matching handler
TEST_F(ContainedTabHandlerRegistryTest,
       ShouldInheritStoragePartition_WithMatchingHandler) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("matching-handler");
  const std::string handler_id = handler->GetId();
  registry.RegisterContainedTabHandler(std::move(handler));

  content::StoragePartitionConfig config =
      content::StoragePartitionConfig::Create(
          browser_context_.get(), handler_id, "partition-name", false);

  EXPECT_TRUE(registry.ShouldInheritStoragePartition(config));
}

// Test ShouldInheritStoragePartition with non-matching handler
TEST_F(ContainedTabHandlerRegistryTest,
       ShouldInheritStoragePartition_WithoutMatchingHandler) {
  auto& registry = GetRegistry();

  content::StoragePartitionConfig config =
      content::StoragePartitionConfig::Create(browser_context_.get(),
                                              "non-existent-handler",
                                              "partition-name", false);

  EXPECT_FALSE(registry.ShouldInheritStoragePartition(config));
}

// Test MaybeInheritStoragePartition with null WebContents
TEST_F(ContainedTabHandlerRegistryTest,
       MaybeInheritStoragePartition_WithNullWebContents) {
  auto& registry = GetRegistry();

  auto result = registry.MaybeInheritStoragePartition(
      static_cast<content::WebContents*>(nullptr));

  EXPECT_FALSE(result.has_value());
}

// Test MaybeInheritStoragePartition with storage partition config
TEST_F(ContainedTabHandlerRegistryTest,
       MaybeInheritStoragePartition_WithMatchingConfig) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("config-handler");
  const std::string handler_id = handler->GetId();
  registry.RegisterContainedTabHandler(std::move(handler));

  content::StoragePartitionConfig config =
      content::StoragePartitionConfig::Create(
          browser_context_.get(), handler_id, "partition-name", false);

  auto result = registry.MaybeInheritStoragePartition(config, nullptr);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->partition_domain(), handler_id);
  EXPECT_EQ(result->partition_name(), "partition-name");
}

// Test MaybeInheritStoragePartition with non-matching config
TEST_F(ContainedTabHandlerRegistryTest,
       MaybeInheritStoragePartition_WithNonMatchingConfig) {
  auto& registry = GetRegistry();

  content::StoragePartitionConfig config =
      content::StoragePartitionConfig::Create(browser_context_.get(),
                                              "non-matching-handler",
                                              "partition-name", false);

  auto result = registry.MaybeInheritStoragePartition(config, nullptr);

  EXPECT_FALSE(result.has_value());
}

// Test MaybeInheritStoragePartition with no config and no site instance
TEST_F(ContainedTabHandlerRegistryTest,
       MaybeInheritStoragePartition_WithNoConfigAndNoSiteInstance) {
  auto& registry = GetRegistry();

  auto result = registry.MaybeInheritStoragePartition(std::nullopt, nullptr);

  EXPECT_FALSE(result.has_value());
}

// Test GetVirtualUrlPrefix with registered handler
TEST_F(ContainedTabHandlerRegistryTest,
       GetVirtualUrlPrefix_WithRegisteredHandler) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("url-handler");
  const std::string handler_id = handler->GetId();
  registry.RegisterContainedTabHandler(std::move(handler));

  std::pair<std::string, std::string> storage_key = {handler_id,
                                                     "my-partition"};

  auto result = registry.GetVirtualUrlPrefix(storage_key);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, handler_id + "+my-partition:");
}

// Test GetVirtualUrlPrefix with unregistered handler
TEST_F(ContainedTabHandlerRegistryTest,
       GetVirtualUrlPrefix_WithUnregisteredHandler) {
  auto& registry = GetRegistry();

  std::pair<std::string, std::string> storage_key = {"unregistered-handler",
                                                     "my-partition"};

  auto result = registry.GetVirtualUrlPrefix(storage_key);

  EXPECT_FALSE(result.has_value());
}

// Test RestoreStoragePartitionKeyFromUrl with valid URL
TEST_F(ContainedTabHandlerRegistryTest,
       RestoreStoragePartitionKeyFromUrl_ValidUrl) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("restore-handler");
  const std::string handler_id = handler->GetId();
  registry.RegisterContainedTabHandler(std::move(handler));

  std::string virtual_url_string =
      handler_id + "+test-partition:https://example.com/path?query=value";
  GURL virtual_url(virtual_url_string);

  std::pair<std::string, std::string> storage_key;
  size_t url_prefix_length = 0;

  auto result = registry.RestoreStoragePartitionKeyFromUrl(
      virtual_url, storage_key, url_prefix_length);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->spec(), "https://example.com/path?query=value");
  EXPECT_EQ(storage_key.first, handler_id);
  EXPECT_EQ(storage_key.second, "test-partition");
  EXPECT_GT(url_prefix_length, 0u);
  EXPECT_EQ(url_prefix_length,
            handler_id.length() + 1 + strlen("test-partition") + 1);
}

// Test RestoreStoragePartitionKeyFromUrl with unregistered handler
TEST_F(ContainedTabHandlerRegistryTest,
       RestoreStoragePartitionKeyFromUrl_UnregisteredHandler) {
  auto& registry = GetRegistry();

  GURL virtual_url("unregistered-handler+partition:https://example.com/");

  std::pair<std::string, std::string> storage_key;
  size_t url_prefix_length = 0;

  auto result = registry.RestoreStoragePartitionKeyFromUrl(
      virtual_url, storage_key, url_prefix_length);

  EXPECT_FALSE(result.has_value());
}

// Test RestoreStoragePartitionKeyFromUrl with invalid format (no separator)
TEST_F(ContainedTabHandlerRegistryTest,
       RestoreStoragePartitionKeyFromUrl_InvalidFormat) {
  auto& registry = GetRegistry();

  GURL virtual_url("https://example.com/");

  std::pair<std::string, std::string> storage_key;
  size_t url_prefix_length = 0;

  auto result = registry.RestoreStoragePartitionKeyFromUrl(
      virtual_url, storage_key, url_prefix_length);

  EXPECT_FALSE(result.has_value());
}

// Test RestoreStoragePartitionKeyFromUrl with too many separators
TEST_F(ContainedTabHandlerRegistryTest,
       RestoreStoragePartitionKeyFromUrl_TooManySeparators) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("multi-sep-handler");
  registry.RegisterContainedTabHandler(std::move(handler));

  GURL virtual_url("part1+part2+part3:https://example.com/");

  std::pair<std::string, std::string> storage_key;
  size_t url_prefix_length = 0;

  auto result = registry.RestoreStoragePartitionKeyFromUrl(
      virtual_url, storage_key, url_prefix_length);

  EXPECT_FALSE(result.has_value());
}

// Test round-trip: GetVirtualUrlPrefix -> RestoreStoragePartitionKeyFromUrl
TEST_F(ContainedTabHandlerRegistryTest, RoundTrip_VirtualUrlPrefixAndRestore) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("roundtrip-handler");
  const std::string handler_id = handler->GetId();
  registry.RegisterContainedTabHandler(std::move(handler));

  std::pair<std::string, std::string> original_storage_key = {handler_id,
                                                              "my-container"};
  std::string original_url = "https://example.com/test/path";

  // Get virtual URL prefix
  auto prefix = registry.GetVirtualUrlPrefix(original_storage_key);
  ASSERT_TRUE(prefix.has_value());

  // Create virtual URL
  GURL virtual_url(*prefix + original_url);

  // Restore from virtual URL
  std::pair<std::string, std::string> restored_storage_key;
  size_t url_prefix_length = 0;
  auto restored_url = registry.RestoreStoragePartitionKeyFromUrl(
      virtual_url, restored_storage_key, url_prefix_length);

  // Verify round-trip
  ASSERT_TRUE(restored_url.has_value());
  EXPECT_EQ(restored_url->spec(), original_url);
  EXPECT_EQ(restored_storage_key.first, original_storage_key.first);
  EXPECT_EQ(restored_storage_key.second, original_storage_key.second);
}

// Test with multiple handlers registered
TEST_F(ContainedTabHandlerRegistryTest, MultipleHandlers_AllRecognized) {
  auto& registry = GetRegistry();

  auto handler1 = CreateMockHandler("handler-one");
  auto handler2 = CreateMockHandler("handler-two");
  auto handler3 = CreateMockHandler("handler-three");

  const std::string id1 = handler1->GetId();
  const std::string id2 = handler2->GetId();
  const std::string id3 = handler3->GetId();

  registry.RegisterContainedTabHandler(std::move(handler1));
  registry.RegisterContainedTabHandler(std::move(handler2));
  registry.RegisterContainedTabHandler(std::move(handler3));

  // Test that all handlers are recognized
  content::StoragePartitionConfig config1 =
      content::StoragePartitionConfig::Create(browser_context_.get(), id1,
                                              "part", false);
  content::StoragePartitionConfig config2 =
      content::StoragePartitionConfig::Create(browser_context_.get(), id2,
                                              "part", false);
  content::StoragePartitionConfig config3 =
      content::StoragePartitionConfig::Create(browser_context_.get(), id3,
                                              "part", false);

  EXPECT_TRUE(registry.ShouldInheritStoragePartition(config1));
  EXPECT_TRUE(registry.ShouldInheritStoragePartition(config2));
  EXPECT_TRUE(registry.ShouldInheritStoragePartition(config3));
}

// Test GetVirtualUrlPrefix with various partition names
TEST_F(ContainedTabHandlerRegistryTest,
       GetVirtualUrlPrefix_VariousPartitionNames) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("test-various");
  const std::string handler_id = handler->GetId();
  registry.RegisterContainedTabHandler(std::move(handler));

  // Test with alphanumeric and hyphens (valid characters)
  std::vector<std::string> valid_names = {"simple", "with-hyphen", "alpha123",
                                          "name-with-123"};

  for (const auto& name : valid_names) {
    std::pair<std::string, std::string> storage_key = {handler_id, name};
    auto result = registry.GetVirtualUrlPrefix(storage_key);
    ASSERT_TRUE(result.has_value()) << "Failed for partition name: " << name;
    EXPECT_EQ(*result, handler_id + "+" + name + ":");
  }
}

// Death tests for DCHECK validation
#if DCHECK_IS_ON()

// Test that registering a handler without proper prefix fails
TEST_F(ContainedTabHandlerRegistryTest,
       RegisterContainedTabHandler_InvalidPrefix_Crashes) {
  auto& registry = GetRegistry();
  auto invalid_handler =
      std::make_unique<MockContainedTabHandler>("invalid-prefix");

  EXPECT_DCHECK_DEATH(
      registry.RegisterContainedTabHandler(std::move(invalid_handler)));
}

// Test that registering a handler with only the prefix fails
TEST_F(ContainedTabHandlerRegistryTest,
       RegisterContainedTabHandler_OnlyPrefix_Crashes) {
  auto& registry = GetRegistry();
  auto invalid_handler = std::make_unique<MockContainedTabHandler>(
      std::string(ContainedTabHandler::kIdPrefix));

  EXPECT_DCHECK_DEATH(
      registry.RegisterContainedTabHandler(std::move(invalid_handler)));
}

#endif  // DCHECK_IS_ON()

// Test RestoreStoragePartitionKeyFromUrl preserves URL components
TEST_F(ContainedTabHandlerRegistryTest,
       RestoreStoragePartitionKeyFromUrl_PreservesUrlComponents) {
  auto& registry = GetRegistry();
  auto handler = CreateMockHandler("preserve-handler");
  const std::string handler_id = handler->GetId();
  registry.RegisterContainedTabHandler(std::move(handler));

  std::string original_url =
      "https://user:pass@example.com:8080/path/"
      "file?query=value&key=data#fragment";
  std::string virtual_url_string = handler_id + "+my-part:" + original_url;
  GURL virtual_url(virtual_url_string);

  std::pair<std::string, std::string> storage_key;
  size_t url_prefix_length = 0;

  auto result = registry.RestoreStoragePartitionKeyFromUrl(
      virtual_url, storage_key, url_prefix_length);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->spec(), original_url);
  EXPECT_EQ(storage_key.first, handler_id);
  EXPECT_EQ(storage_key.second, "my-part");
}

}  // namespace containers
