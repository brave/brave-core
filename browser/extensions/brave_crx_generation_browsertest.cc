/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/install_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/crx_file/crx_verifier.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "crypto/rsa_private_key.h"
#include "crypto/sha2.h"
#include "extensions/browser/crx_file_info.h"
#include "extensions/browser/extension_creator.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"

namespace extensions {

namespace {
// Generate a publish key hash from .pem file in the format used in
// crx_verifier.cc.
std::vector<uint8_t> GetPublicKeyHash(const base::FilePath& pem_path) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  std::string private_key_contents;
  if (!base::ReadFileToString(pem_path, &private_key_contents))
    return {};
  std::string private_key_bytes;
  if (!Extension::ParsePEMKeyBytes(private_key_contents, &private_key_bytes))
    return {};

  auto private_key = crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(
      std::vector<uint8_t>(private_key_bytes.begin(), private_key_bytes.end()));
  if (!private_key)
    return {};

  std::vector<uint8_t> public_key;
  private_key->ExportPublicKey(&public_key);

  std::vector<uint8_t> key_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(std::string(public_key.begin(), public_key.end()),
                           key_hash.data(), key_hash.size());
  return key_hash;
}

}  // namespace

class InstallCrxFileWaiter : public extensions::InstallObserver {
 public:
  explicit InstallCrxFileWaiter(Profile* profile) : profile_(profile) {
    InstallTracker::Get(profile_)->AddObserver(this);
  }

  ~InstallCrxFileWaiter() override {
    InstallTracker::Get(profile_)->RemoveObserver(this);
  }

  void OnFinishCrxInstall(content::BrowserContext* context,
                          const extensions::CrxInstaller& installer,
                          const std::string& extension_id,
                          bool success) override {
    did_install_extension_ = success;
    run_loop_.Quit();
  }

  bool WaitForInstallation() {
    run_loop_.Run();
    return did_install_extension_;
  }

 private:
  raw_ptr<Profile> profile_ = nullptr;
  bool did_install_extension_ = false;
  base::RunLoop run_loop_;
};

class BraveCrxGenerationTest : public InProcessBrowserTest {
 public:
  BraveCrxGenerationTest() {
    CHECK(temp_directory_.CreateUniqueTempDir());
    brave::RegisterPathProvider();
  }

  bool InstallExtension(const base::FilePath& crx_path,
                        crx_file::VerifierFormat format) {
    auto installer = CrxInstaller::CreateSilent(
        ExtensionSystem::Get(browser()->profile())->extension_service());
    installer->set_allow_silent_install(true);
    installer->set_install_cause(extension_misc::INSTALL_CAUSE_USER_DOWNLOAD);
    installer->set_creation_flags(Extension::FROM_WEBSTORE);

    InstallCrxFileWaiter waiter(browser()->profile());
    installer->InstallCrxFile(CRXFileInfo(crx_path, format));
    return waiter.WaitForInstallation();
  }

  base::FilePath GetTestDataDir() {
    base::FilePath test_data_dir;
    base::ScopedAllowBlockingForTesting allow_blocking;
    CHECK(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));
    return test_data_dir;
  }

  base::FilePath CreateTestCrx() {
    base::ScopedAllowBlockingForTesting allow_blocking;

    const auto extension_dir =
        GetTestDataDir().AppendASCII("extensions/trivial_extension");

    const auto private_key_path =
        GetTestDataDir().AppendASCII("extensions/trivial_extension.pem");
    const auto crx_path = temp_directory_.GetPath().AppendASCII("test.crx");

    ExtensionCreator creator;
    if (!creator.Run(extension_dir, crx_path, private_key_path,
                     base::FilePath(), ExtensionCreator::kOverwriteCRX)) {
      LOG(ERROR) << creator.error_message();
      return base::FilePath();
    }
    return crx_path;
  }

  base::ScopedTempDir temp_directory_;
};

IN_PROC_BROWSER_TEST_F(BraveCrxGenerationTest,
                       CrxVerificationWithoutPublisherProof) {
  // Generate CRX without the publisher proof (only standard developer's
  // signature).
  const auto crx_path = CreateTestCrx();

  // Extension should be installable because a publisher proof is not required.
  EXPECT_TRUE(InstallExtension(crx_path, crx_file::VerifierFormat::CRX3));
}

// Check the browser is able to generate .crx files (extensions and components)
// with a valid publisher proof.
IN_PROC_BROWSER_TEST_F(BraveCrxGenerationTest,
                       CrxVerificationWithPublisherProof) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  {
    // Generate CRX without publisher proof.
    const auto crx_path = CreateTestCrx();

    // Extension should fail verification with `CRX_REQUIRED_PROOF_MISSING' in
    // the console.
    EXPECT_FALSE(InstallExtension(
        crx_path, crx_file::VerifierFormat::CRX3_WITH_PUBLISHER_PROOF));
  }

  const auto publisher_test_key_path =
      GetTestDataDir().AppendASCII("extensions/test_publisher_proof_key.pem");

  // Add the test key to the command line for using in crx generating process.
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      "brave-extension-publisher-key", publisher_test_key_path);

  // Also add the alternative test key.
  const auto alt_publisher_test_key_path = GetTestDataDir().AppendASCII(
      "extensions/test_publisher_proof_key_alt.pem");
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      "brave-extension-publisher-key-alt", alt_publisher_test_key_path);

  // Make sure the extension now passes verification for each test key.
  for (auto test_key : {publisher_test_key_path, alt_publisher_test_key_path}) {
    const auto public_key_hash = GetPublicKeyHash(test_key);
    ASSERT_GT(public_key_hash.size(), 0u);
    crx_file::SetBravePublisherKeyHashForTesting(public_key_hash);

    const auto crx_path = CreateTestCrx();

    EXPECT_TRUE(InstallExtension(
        crx_path, crx_file::VerifierFormat::CRX3_WITH_PUBLISHER_PROOF));
  }
}

}  // namespace extensions
