/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using extensions::BraveComponentLoader;

class BraveComponentLoaderTest : public extensions::ExtensionFunctionalTest,
  public BraveComponentLoader::TestingCallbacks {
 public:
  BraveComponentLoaderTest() : pdf_extension_action_(TestingCallbacks::NONE) {}
  ~BraveComponentLoaderTest() override = default;

 protected:
  void SetUpOnMainThread() override {
    extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile())->extension_service();
    DCHECK(service);
    BraveComponentLoader* loader =
      static_cast<BraveComponentLoader*>(service->component_loader());
    loader->set_testing_callbacks(this);
    // Do this again so OnComponentRegistered callback will be called.
    loader->AddDefaultComponentExtensions(false);
  }

  // BraveComponentLoader::TestingCallbacks
  void OnPdfExtensionAction(
      TestingCallbacks::PdfExtensionAction action) override {
    pdf_extension_action_ = action;
  }

  MOCK_METHOD1(OnComponentRegistered, void(std::string));

  TestingCallbacks::PdfExtensionAction pdf_extension_action() {
    return pdf_extension_action_;
  }

 private:
  TestingCallbacks::PdfExtensionAction pdf_extension_action_;
};

class BraveIPFSExtensionTest: public BraveComponentLoaderTest {
 public:
  BraveIPFSExtensionTest() {}
  ~BraveIPFSExtensionTest() override = default;
};

IN_PROC_BROWSER_TEST_F(BraveIPFSExtensionTest, DisabledByDefault) {
  ASSERT_FALSE(
      profile()->GetPrefs()->GetBoolean(kIPFSCompanionEnabled));
  EXPECT_CALL(*this, OnComponentRegistered(_)).Times(AnyNumber());
  EXPECT_CALL(*this,
      OnComponentRegistered(ipfs_companion_extension_id)).Times(0);
}

IN_PROC_BROWSER_TEST_F(BraveIPFSExtensionTest,
                       PRE_IPFSCompanionEnabledDoesRegisterComponent) {
  profile()->GetPrefs()->SetBoolean(kIPFSCompanionEnabled, true);
}

IN_PROC_BROWSER_TEST_F(BraveIPFSExtensionTest,
                       IPFSCompanionEnabledDoesRegisterComponent) {
  ASSERT_TRUE(
      profile()->GetPrefs()->GetBoolean(kIPFSCompanionEnabled));
  EXPECT_CALL(*this, OnComponentRegistered(_)).Times(AnyNumber());
  EXPECT_CALL(*this, OnComponentRegistered(ipfs_companion_extension_id));
}

class BravePDFExtensionTest : public BraveComponentLoaderTest {
 public:
  BravePDFExtensionTest() {}
  ~BravePDFExtensionTest() override = default;

  void SetDownloadPDFs(bool value) {
    DCHECK(browser());
    profile()->GetPrefs()->SetBoolean(prefs::kPluginsAlwaysOpenPdfExternally,
                                      value);
  }
};

IN_PROC_BROWSER_TEST_F(BravePDFExtensionTest, ToggleDownloadPDFs) {
  // Set preference to always download PDFs.
  SetDownloadPDFs(true);
  EXPECT_EQ(TestingCallbacks::WILL_REMOVE, pdf_extension_action());

  // Toggle the preference to view PDFs in the browser.
  SetDownloadPDFs(false);
  EXPECT_EQ(TestingCallbacks::WILL_ADD, pdf_extension_action());
}

class BravePDFExtensionDisabledTest : public BravePDFExtensionTest {
 public:
  BravePDFExtensionDisabledTest() = default;
  ~BravePDFExtensionDisabledTest() override = default;

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionFunctionalTest::SetUpCommandLine(command_line);
    // Disable loading of our PDF extension.
    command_line->AppendSwitch(switches::kDisablePDFJSExtension);
  }
};

IN_PROC_BROWSER_TEST_F(BravePDFExtensionDisabledTest, ToggleDownloadPDFs) {
  // Set preference to always download PDFs.
  SetDownloadPDFs(true);
  EXPECT_EQ(TestingCallbacks::WILL_REMOVE, pdf_extension_action());

  // Toggle the preference to view PDFs in the browser.
  SetDownloadPDFs(false);
  EXPECT_EQ(TestingCallbacks::WILL_REMOVE, pdf_extension_action());
}
