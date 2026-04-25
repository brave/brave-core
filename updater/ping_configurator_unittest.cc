/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/ping_configurator.h"

#include "brave/components/update_client/test_util.h"
#include "components/update_client/protocol_handler.h"
#include "components/update_client/protocol_serializer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

TEST(PingConfigurator, UsesPrivacyPreservingProtocolSerializer) {
  auto configurator = CreatePingConfigurator();

  auto factory = configurator->GetProtocolHandlerFactory();
  ASSERT_TRUE(factory);

  auto serializer = factory->CreateSerializer();
  ASSERT_TRUE(serializer);

  EXPECT_TRUE(update_client::StripsPrivacySensitiveData(*serializer));
}

}  // namespace updater
