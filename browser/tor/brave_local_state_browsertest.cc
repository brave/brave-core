/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "gtest/gtest.h"

using BraveLocalStateBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, BasicTest) {
  // Tor is enabled by default.
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled());

  // No bridges by default.
  auto bridges_config = TorProfileServiceFactory::GetTorBridgesConfig();
  EXPECT_FALSE(bridges_config.use_bridges);
  EXPECT_EQ(tor::BridgesConfig::BuiltinType::kNone, bridges_config.use_builtin);
  EXPECT_TRUE(bridges_config.bridges.empty());
}

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, ChangeTest) {
  TorProfileServiceFactory::SetTorDisabled(false);
  EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled());

  tor::BridgesConfig bridges_config;
  bridges_config.use_bridges = true;
  EXPECT_EQ(tor::BridgesConfig::BuiltinType::kNone, bridges_config.use_builtin);
  bridges_config.bridges.push_back("bridge1");
  bridges_config.bridges.push_back("bridge2");
  bridges_config.bridges.push_back("bridge3");

  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);
  EXPECT_EQ(bridges_config.ToValue(),
            TorProfileServiceFactory::GetTorBridgesConfig().ToValue());
}
