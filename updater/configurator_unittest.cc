/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/configurator.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "brave/components/update_client/test_util.h"
#include "chrome/updater/external_constants.h"
#include "chrome/updater/persisted_data.h"
#include "chrome/updater/prefs_impl.h"
#include "chrome/updater/updater_scope.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/update_client/protocol_handler.h"
#include "components/update_client/protocol_serializer.h"
#include "components/update_client/update_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace updater {

TEST(Configurator, UsesPrivacyPreservingProtocolSerializer) {
  base::test::TaskEnvironment task_environment;

  auto pref_service = std::make_unique<TestingPrefServiceSimple>();
  update_client::RegisterPrefs(pref_service->registry());
  RegisterPersistedDataPrefs(pref_service->registry());

  auto prefs = base::MakeRefCounted<UpdaterPrefsImpl>(base::FilePath(), nullptr,
                                                      std::move(pref_service));
  auto external_constants = CreateExternalConstants();

  auto configurator = base::MakeRefCounted<Configurator>(
      prefs, external_constants, UpdaterScope::kUser);
  auto factory = configurator->GetProtocolHandlerFactory();
  ASSERT_TRUE(factory);

  auto serializer = factory->CreateSerializer();
  ASSERT_TRUE(serializer);

  EXPECT_TRUE(update_client::StripsPrivacySensitiveData(*serializer));
}

}  // namespace updater
