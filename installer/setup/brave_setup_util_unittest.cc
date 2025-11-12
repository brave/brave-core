#include "brave/installer/setup/brave_setup_util.h"

#include <memory>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/test_reg_util_win.h"
#include "base/version.h"
#include "chrome/installer/setup/installer_state.h"
#include "chrome/installer/setup/setup_constants.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/util_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A test fixture that configures an InstallationState and an InstallerState
// with a product being updated.
class FindArchiveToPatchTest : public testing::Test {
 public:
  FindArchiveToPatchTest(const FindArchiveToPatchTest&) = delete;
  FindArchiveToPatchTest& operator=(const FindArchiveToPatchTest&) = delete;

 protected:
  class FakeInstallationState : public installer::InstallationState {};

  class FakeProductState : public installer::ProductState {
   public:
    static FakeProductState* FromProductState(const ProductState* product) {
      return static_cast<FakeProductState*>(const_cast<ProductState*>(product));
    }

    void set_version(const base::Version& version) {
      if (version.IsValid())
        version_ = std::make_unique<base::Version>(version);
      else
        version_.reset();
    }

    void set_uninstall_command(const base::CommandLine& uninstall_command) {
      uninstall_command_ = uninstall_command;
    }
  };

  FindArchiveToPatchTest() = default;

  void SetUp() override {
    ASSERT_TRUE(test_dir_.CreateUniqueTempDir());
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_LOCAL_MACHINE));
    product_version_ = base::Version("30.0.1559.0");
    max_version_ = base::Version("47.0.1559.0");

    // Install the product according to the version.
    original_state_ = std::make_unique<FakeInstallationState>();
    InstallProduct();

    // Prepare to update the product in the temp dir.
    installer_state_ = std::make_unique<installer::InstallerState>(
        kSystemInstall_ ? installer::InstallerState::SYSTEM_LEVEL
                        : installer::InstallerState::USER_LEVEL);
    installer_state_->set_target_path_for_testing(test_dir_.GetPath());

    // Create archives in the two version dirs.
    ASSERT_TRUE(
        base::CreateDirectory(GetProductVersionArchivePath().DirName()));
    ASSERT_TRUE(base::WriteFile(GetProductVersionArchivePath(), "a"));
    ASSERT_TRUE(base::CreateDirectory(GetMaxVersionArchivePath().DirName()));
    ASSERT_TRUE(base::WriteFile(GetMaxVersionArchivePath(), "b"));
  }

  void TearDown() override { original_state_.reset(); }

  base::FilePath GetArchivePath(const base::Version& version) const {
    return test_dir_.GetPath()
        .AppendASCII(version.GetString())
        .Append(installer::kInstallerDir)
        .Append(installer::kChromeArchive);
  }

  base::FilePath GetMaxVersionArchivePath() const {
    return GetArchivePath(max_version_);
  }

  base::FilePath GetProductVersionArchivePath() const {
    return GetArchivePath(product_version_);
  }

  void InstallProduct() {
    FakeProductState* product = FakeProductState::FromProductState(
        original_state_->GetNonVersionedProductState(kSystemInstall_));

    product->set_version(product_version_);
    base::CommandLine uninstall_command(
        test_dir_.GetPath()
            .AppendASCII(product_version_.GetString())
            .Append(installer::kInstallerDir)
            .Append(installer::kSetupExe));
    uninstall_command.AppendSwitch(installer::switches::kUninstall);
    product->set_uninstall_command(uninstall_command);
  }

  void UninstallProduct() {
    FakeProductState::FromProductState(
        original_state_->GetNonVersionedProductState(kSystemInstall_))
        ->set_version(base::Version());
  }

  static const bool kSystemInstall_;
  base::ScopedTempDir test_dir_;
  base::Version product_version_;
  base::Version max_version_;
  std::unique_ptr<FakeInstallationState> original_state_;
  std::unique_ptr<installer::InstallerState> installer_state_;

 private:
  registry_util::RegistryOverrideManager registry_override_manager_;
};

const bool FindArchiveToPatchTest::kSystemInstall_ = false;

}  // namespace

// Test that the path to the advertised product version is found.
TEST_F(FindArchiveToPatchTest, ProductVersionFound) {
  base::FilePath patch_source(installer::FindArchiveToPatch(
      *original_state_, *installer_state_, base::Version()));
  EXPECT_EQ(GetProductVersionArchivePath().value(), patch_source.value());
}

// Test that the path to the max version is found if the advertised version is
// missing.
TEST_F(FindArchiveToPatchTest, MaxVersionFound) {
  // The patch file is absent.
  ASSERT_TRUE(base::DeleteFile(GetProductVersionArchivePath()));
  base::FilePath patch_source(installer::FindArchiveToPatch(
      *original_state_, *installer_state_, base::Version()));
  EXPECT_EQ(GetMaxVersionArchivePath().value(), patch_source.value());

  // The product doesn't appear to be installed, so the max version is found.
  UninstallProduct();
  patch_source = installer::FindArchiveToPatch(
      *original_state_, *installer_state_, base::Version());
  EXPECT_EQ(GetMaxVersionArchivePath().value(), patch_source.value());
}

// Test that an empty path is returned if no version is found.
TEST_F(FindArchiveToPatchTest, NoVersionFound) {
  // The product doesn't appear to be installed and no archives are present.
  UninstallProduct();
  ASSERT_TRUE(base::DeleteFile(GetProductVersionArchivePath()));
  ASSERT_TRUE(base::DeleteFile(GetMaxVersionArchivePath()));

  base::FilePath patch_source(installer::FindArchiveToPatch(
      *original_state_, *installer_state_, base::Version()));
  EXPECT_EQ(base::FilePath::StringType(), patch_source.value());
}

TEST_F(FindArchiveToPatchTest, DesiredVersionFound) {
  base::FilePath patch_source1(installer::FindArchiveToPatch(
      *original_state_, *installer_state_, product_version_));
  EXPECT_EQ(GetProductVersionArchivePath().value(), patch_source1.value());
  base::FilePath patch_source2(installer::FindArchiveToPatch(
      *original_state_, *installer_state_, max_version_));
  EXPECT_EQ(GetMaxVersionArchivePath().value(), patch_source2.value());
}

TEST_F(FindArchiveToPatchTest, DesiredVersionNotFound) {
  base::FilePath patch_source(installer::FindArchiveToPatch(
      *original_state_, *installer_state_, base::Version("1.2.3.4")));
  EXPECT_EQ(base::FilePath().value(), patch_source.value());
}