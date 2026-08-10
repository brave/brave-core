// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/command_line_container.h"

#include <memory>
#include <string>
#include <variant>

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/core/browser/container_specifier.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/containers_test_utils.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/browser/prefs_registration.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/common/switches.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class CommandLineContainerTest : public testing::Test {
 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kContainers);
    RegisterProfilePrefs(prefs_.registry());

    auto delegate =
        std::make_unique<testing::NiceMock<MockContainersServiceDelegate>>();
    service_ = std::make_unique<ContainersService>(
        &prefs_, /*is_off_the_record=*/false, std::move(delegate));
  }

  void TearDown() override {
    service_->Shutdown();
    service_.reset();
  }

  ContainerSpecifier GetSpecifier(const base::CommandLine& command_line) {
    return GetContainerSpecifierForCommandLineTabs(command_line,
                                                   service_.get());
  }

  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<ContainersService> service_;
};

TEST_F(CommandLineContainerTest, NoSwitchesReturnsEmpty) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  EXPECT_TRUE(
      std::holds_alternative<std::monostate>(GetSpecifier(command_line)));
}

TEST_F(CommandLineContainerTest, ContainerSwitchResolvesByName) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kContainer, "Work");

  const ContainerSpecifier specifier = GetSpecifier(command_line);
  ASSERT_TRUE(std::holds_alternative<ContainerName>(specifier));
  EXPECT_EQ("Work", std::get<ContainerName>(specifier).value());
}

TEST_F(CommandLineContainerTest, TemporaryContainerSwitchCreatesTemporary) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitch(switches::kTemporaryContainer);

  const ContainerSpecifier specifier = GetSpecifier(command_line);
  ASSERT_TRUE(std::holds_alternative<ContainerId>(specifier));
  const std::string& id = std::get<ContainerId>(specifier).value();
  EXPECT_TRUE(IsTemporaryContainerId(id)) << id;
  // The temporary container is persisted so a later launch can reuse it.
  EXPECT_TRUE(GetLocallyUsedContainerFromPrefs(prefs_, id));
}

TEST_F(CommandLineContainerTest, NamedTemporaryContainerIsReused) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitch(switches::kTemporaryContainer);
  command_line.AppendSwitchASCII(switches::kContainer, "Named");

  const ContainerSpecifier first = GetSpecifier(command_line);
  ASSERT_TRUE(std::holds_alternative<ContainerId>(first));
  const std::string first_id = std::get<ContainerId>(first).value();
  EXPECT_TRUE(IsTemporaryContainerId(first_id)) << first_id;

  auto container = service_->GetRuntimeContainerById(first_id);
  ASSERT_TRUE(container);
  EXPECT_EQ("Named", container->name);

  // The same name resolves to the same temporary container on a later launch.
  const ContainerSpecifier second = GetSpecifier(command_line);
  ASSERT_TRUE(std::holds_alternative<ContainerId>(second));
  EXPECT_EQ(first_id, std::get<ContainerId>(second).value());
}

}  // namespace containers
