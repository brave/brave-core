// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/values.h"
#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_installer.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/install_signer.h"
#include "chrome/browser/extensions/install_verifier.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

struct BraveExtensionsManifestV2Test
    : public extensions::ExtensionServiceTestBase {
 public:
  void SetUp() override {
    extensions::ExtensionServiceTestBase::SetUp();

    InitializeExtensionService(ExtensionServiceInitParams());
  }

  void InitVerifier() {
    extensions::InstallSignature signature = {};
    auto dict = signature.ToDict();
    extensions::ExtensionPrefs::Get(profile())->SetInstallSignature(&dict);
    extensions::InstallVerifier::Get(profile())->Init();
  }

 private:
  extensions::ScopedInstallVerifierBypassForTest force_install_verification_{
      extensions::ScopedInstallVerifierBypassForTest::kForceOn};
};

TEST_F(BraveExtensionsManifestV2Test, CheckInstallVerifier) {
  struct {
    const char* extension_id;
    bool expected_must_remain_disabled;
    extensions::disable_reason::DisableReason expected_reason;
  } test_cases[] = {{extensions_mv2::kNoScriptId, false,
                     extensions::disable_reason::DISABLE_NONE},
                    {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", true,
                     extensions::disable_reason::DISABLE_NOT_VERIFIED}};

  InitVerifier();

  for (const auto& test : test_cases) {
    SCOPED_TRACE(::testing::Message() << test.extension_id);

    auto extension =
        extensions::ExtensionBuilder("test")
            .SetID(test.extension_id)
            .AddFlags(extensions::Extension::FROM_WEBSTORE)
            .SetLocation(extensions::mojom::ManifestLocation::kExternalPolicy)
            .Build();

    auto* install_verifier = extensions::InstallVerifier::Get(profile());

    auto disable_reason = extensions::disable_reason::DISABLE_NONE;
    EXPECT_EQ(
        test.expected_must_remain_disabled,
        install_verifier->MustRemainDisabled(extension.get(), &disable_reason));
    EXPECT_EQ(test.expected_reason, disable_reason);
  }
}
