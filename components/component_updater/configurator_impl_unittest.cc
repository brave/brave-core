/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/component_updater/configurator_impl.h"

#include "brave/components/update_client/test_util.h"
#include "components/update_client/command_line_config_policy.h"
#include "components/update_client/protocol_handler.h"
#include "components/update_client/protocol_serializer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace component_updater {

TEST(BraveComponentUpdaterConfiguratorImplTest,
     UsesPrivacyPreservingProtocolSerializer) {
  auto configurator = std::make_unique<ConfiguratorImpl>(
      update_client::CommandLineConfigPolicy(), false);

  auto factory = configurator->GetProtocolHandlerFactory();
  ASSERT_TRUE(factory);

  auto serializer = factory->CreateSerializer();
  ASSERT_TRUE(serializer);

  EXPECT_TRUE(update_client::StripsPrivacySensitiveData(*serializer));
}

}  // namespace component_updater
