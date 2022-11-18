/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/extensions_import_helpers.h"

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "brave/browser/importer/test_storage_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/test/base/testing_profile.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/common/constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class ExtensionsImportHelperstUnitTest : public testing::Test {
 public:
  ExtensionsImportHelperstUnitTest() {}
  ~ExtensionsImportHelperstUnitTest() override {}

  void SetUp() override {
    TestingProfile::Builder profile_builder;
    EXPECT_TRUE(brave_profile_dir_.CreateUniqueTempDir());
    profile_builder.SetPath(GetProductProfilePath("Brave"));
    profile_ = profile_builder.Build();
  }

  base::FilePath GetProductProfilePath(const std::string& product) {
    return brave_profile_dir_.GetPath()
        .AppendASCII("extensions")
        .AppendASCII("import")
        .AppendASCII(product);
  }
  base::FilePath GetExtensionLocalSettingsPath(const std::string& product,
                                               const std::string& id) {
    return GetProductProfilePath(product)
        .Append(extensions::kLocalExtensionSettingsDirectoryName)
        .AppendASCII(id);
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir brave_profile_dir_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(ExtensionsImportHelperstUnitTest, ImportStorages) {
  for (auto i = 0; i < 3; i++) {
    const std::string id = "id" + std::to_string(i);
    brave::CreateTestingStore(GetExtensionLocalSettingsPath("Chrome", id), id,
                              {{"a", "b"}, {"c", "d"}, {"id", id}});
  }
  brave::CreateTestingStore(GetExtensionLocalSettingsPath("Brave", "id0"),
                            "id0", {{"a", "a"}, {"c", "c"}, {"id", "id0"}});
  {
    base::RunLoop loop;
    extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
        FROM_HERE, base::BindLambdaForTesting([&]() {
          ImportStorages(GetProductProfilePath("Chrome"),
                         GetProductProfilePath("Brave"), {"id0", "id2"});
        }),
        loop.QuitClosure());
    loop.Run();
  }
  EXPECT_FALSE(ReadStore(GetExtensionLocalSettingsPath("Brave", "id1"), "id1"));

  EXPECT_EQ(ReadStore(GetExtensionLocalSettingsPath("Brave", "id0"), "id0"),
            base::JSONReader::Read(R"({
    "a": "a",
    "c": "c",
    "id": "id0"
  })"));

  EXPECT_EQ(ReadStore(GetExtensionLocalSettingsPath("Brave", "id2"), "id2"),
            base::JSONReader::Read(R"({
    "a": "b",
    "c": "d",
    "id": "id2"
  })"));

  {
    base::RunLoop loop;
    extensions::GetExtensionFileTaskRunner()->PostTaskAndReply(
        FROM_HERE, base::BindLambdaForTesting([&]() {
          RemoveExtensionsSettings(GetProductProfilePath("Brave"), "id2");
        }),
        loop.QuitClosure());
    loop.Run();
  }
  EXPECT_FALSE(ReadStore(GetExtensionLocalSettingsPath("Brave", "id2"), "id2"));
}

}  // namespace brave
