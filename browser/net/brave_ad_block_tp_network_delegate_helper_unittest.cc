/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/component_updater/brave_component_updater_delegate.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/test/base/testing_brave_browser_process.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave::ResponseCallback;

TEST(BraveAdBlockTPNetworkDelegateHelperTest, NoChangeURL) {
  const GURL url("https://bradhatesprimes.brave.com/composite_numbers_ftw");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  int rc =
      OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, EmptyRequestURL) {
  auto request_info = std::make_shared<brave::BraveRequestInfo>(GURL());
  int rc =
      OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, DevToolURL) {
  const GURL url("devtools://devtools/");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->initiator_url =
      GURL("devtools://devtools/bundled/root/root.js");
  int rc =
      OnBeforeURLRequest_AdBlockTPPreWork(ResponseCallback(), request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::OK);
}

// This namespace is copied from
// components/brave_shields/browser/ad_block_service.cc
//
// It's required unless we can call `AdBlockService::Start()`, see below for
// more info.
namespace {

// Extracts the start and end characters of a domain from a hostname.
// Required for correct functionality of adblock-rust.
void AdBlockServiceDomainResolver(const char* host,
                                  uint32_t* start,
                                  uint32_t* end) {
  const auto host_str = std::string(host);
  const auto domain = net::registry_controlled_domains::GetDomainAndRegistry(
      host_str,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  const size_t match = host_str.rfind(domain);
  if (match != std::string::npos) {
    *start = match;
    *end = match + domain.length();
  } else {
    *start = 0;
    *end = host_str.length();
  }
}

}  // namespace

// Serves as `next_callback` for the request handler
void OnSuccess() {}

TEST(BraveAdBlockTPNetworkDelegateHelperTest, SimpleBlocking) {
  content::BrowserTaskEnvironment task_environment;

  // It appears that g_browser_process automatically gets created for unit
  // tests. It should be possible to make that work for g_brave_browser_process
  // as well.
  TestingBraveBrowserProcess::CreateInstance();

  auto brave_component_updater_delegate =
      std::make_unique<brave::BraveComponentUpdaterDelegate>();
  auto adblock_service = brave_shields::AdBlockServiceFactory(
      brave_component_updater_delegate.get());

  // This would still have to be destroyed/reset after the test finishes,
  // perhaps using scoped classes
  TestingBraveBrowserProcess::GetGlobal()->SetAdBlockService(
      std::move(adblock_service));

  // Normally called through AdBlockService::Start(), which requires
  // registration of the extension
  adblock::SetDomainResolver(AdBlockServiceDomainResolver);

  // Required un-protecting the ResetForTest method, since this didn't work as a
  // friend class
  g_brave_browser_process->ad_block_service()->ResetForTest(
      "||brave.com/test.txt", "");

  const GURL url("https://brave.com/test.txt");
  auto request_info = std::make_shared<brave::BraveRequestInfo>(url);
  request_info->request_identifier = 1;
  request_info->resource_type = blink::mojom::ResourceType::kScript;
  request_info->initiator_url = GURL("https://brave.com");
  int rc =
      OnBeforeURLRequest_AdBlockTPPreWork(base::Bind(&OnSuccess), request_info);
  EXPECT_TRUE(request_info->new_url_spec.empty());
  EXPECT_EQ(rc, net::ERR_IO_PENDING);

  task_environment.RunUntilIdle();

  EXPECT_EQ(request_info->blocked_by, brave::kAdBlocked);
}
