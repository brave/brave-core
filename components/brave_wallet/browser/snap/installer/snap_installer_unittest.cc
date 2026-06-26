/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer.h"

#include <memory>
#include <optional>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SnapInstallerTest : public testing::Test {
 public:
  void SetUp() override {
    SnapRegistry::RegisterProfilePrefs(prefs_.registry());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    data_provider_ =
        std::make_unique<SnapDataProvider>(temp_dir_.GetPath(), prefs_);
    installer_ = std::make_unique<SnapInstaller>(
        *data_provider_, url_loader_factory_.GetSafeWeakWrapper());
  }

  void TearDown() override {
    // Destroy the installer so any prepared InstallContext temp dirs post their
    // async cleanup, then flush it.
    installer_.reset();
    data_provider_.reset();
    task_environment_.RunUntilIdle();
  }

 protected:
  // Serves npm metadata + tarball responses for |snap_id|.
  void RegisterSnap(const std::string& snap_id,
                    const std::string& version,
                    const std::string& bundle,
                    const std::string& manifest_json,
                    const std::string& bundle_file_path = "dist/bundle.js") {
    const std::string package = snap_id.substr(4);  // strip "npm:"
    const std::string metadata_url =
        "https://registry.npmjs.org/" + package + "/" + version;
    const std::string tarball_url =
        "https://registry.npmjs.org/" + package + "/-/pkg-" + version + ".tgz";
    url_loader_factory_.AddResponse(metadata_url,
                                    MakeNpmRegistryMetadataJson(tarball_url));
    url_loader_factory_.AddResponse(
        tarball_url, BuildSnapTarball(manifest_json, bundle, bundle_file_path));
  }

  // Serves a valid, source-only snap and returns its bundle bytes.
  std::string RegisterValidSnap(const std::string& snap_id = "npm:test-snap",
                                const std::string& version = "1.0.0") {
    const std::string bundle = "export const onRpcRequest = () => 42;";
    TestSnapManifestOptions options;
    options.shasum = ComputeSnapBundleShasum(bundle);
    RegisterSnap(snap_id, version, bundle, MakeSnapManifestJson(options));
    return bundle;
  }

  base::expected<mojom::SnapInstallDataPtr, std::string> Prepare(
      const std::string& snap_id,
      const std::string& version) {
    base::test::TestFuture<
        base::expected<mojom::SnapInstallDataPtr, std::string>>
        future;
    installer_->PrepareInstall(snap_id, version, future.GetCallback());
    return future.Take();
  }

  base::expected<void, std::string> Finish(const std::string& snap_id) {
    base::test::TestFuture<base::expected<void, std::string>> future;
    installer_->FinishInstall(snap_id, future.GetCallback());
    return future.Take();
  }

  std::optional<std::string> ReadBundle(const std::string& snap_id) {
    base::test::TestFuture<std::optional<std::string>> future;
    installer_->GetSnapBundle(snap_id, future.GetCallback());
    return future.Take();
  }

  bool IsInstalled(const std::string& snap_id) {
    base::test::TestFuture<bool> future;
    data_provider_->IsInstalled(snap_id, future.GetCallback());
    return future.Get();
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<SnapDataProvider> data_provider_;
  std::unique_ptr<SnapInstaller> installer_;
};

TEST_F(SnapInstallerTest, PrepareInstallHappyPath) {
  RegisterValidSnap("npm:test-snap", "1.0.0");

  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_TRUE(result.has_value()) << result.error();
  const auto& data = result.value();
  EXPECT_EQ(data->snap_id, "npm:test-snap");
  EXPECT_EQ(data->version, "1.0.0");
  EXPECT_GT(data->bundle_size_bytes, 0u);
  ASSERT_TRUE(data->manifest);
  EXPECT_THAT(data->manifest->permissions, testing::ElementsAre("snap_dialog"));

  // PrepareInstall does not persist; the registry is still empty.
  EXPECT_FALSE(IsInstalled("npm:test-snap"));
}

TEST_F(SnapInstallerTest, FinishInstallPersistsBundleAndRegistry) {
  const std::string bundle = RegisterValidSnap("npm:test-snap", "1.0.0");
  ASSERT_TRUE(Prepare("npm:test-snap", "1.0.0").has_value());

  auto result = Finish("npm:test-snap");
  ASSERT_TRUE(result.has_value()) << result.error();

  EXPECT_TRUE(IsInstalled("npm:test-snap"));
  auto read = ReadBundle("npm:test-snap");
  ASSERT_TRUE(read);
  EXPECT_EQ(*read, bundle);
}

TEST_F(SnapInstallerTest, AbortInstallDropsPreparedContext) {
  RegisterValidSnap("npm:test-snap", "1.0.0");
  ASSERT_TRUE(Prepare("npm:test-snap", "1.0.0").has_value());

  installer_->AbortInstall("npm:test-snap");

  auto result = Finish("npm:test-snap");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "snap not prepared");
}

TEST_F(SnapInstallerTest, FinishWithoutPrepareFails) {
  auto result = Finish("npm:not-prepared");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "snap not prepared");
}

TEST_F(SnapInstallerTest, PrepareWhileAlreadyPreparedFails) {
  RegisterValidSnap("npm:test-snap", "1.0.0");
  ASSERT_TRUE(Prepare("npm:test-snap", "1.0.0").has_value());

  // A second prepare while the first is still held returns an error.
  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "already_preparing");
}

TEST_F(SnapInstallerTest, MetadataFetchFailureFails) {
  url_loader_factory_.AddResponse("https://registry.npmjs.org/test-snap/1.0.0",
                                  "", net::HTTP_NOT_FOUND);
  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Failed to fetch snap metadata");
}

TEST_F(SnapInstallerTest, InvalidMetadataJsonFails) {
  url_loader_factory_.AddResponse("https://registry.npmjs.org/test-snap/1.0.0",
                                  "this is not json");
  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Invalid snap metadata JSON");
}

TEST_F(SnapInstallerTest, MissingTarballUrlFails) {
  url_loader_factory_.AddResponse("https://registry.npmjs.org/test-snap/1.0.0",
                                  R"({"dist": {}})");
  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Missing 'dist.tarball' in snap metadata");
}

TEST_F(SnapInstallerTest, TarballDownloadFailureFails) {
  const std::string tarball_url =
      "https://registry.npmjs.org/test-snap/-/pkg-1.0.0.tgz";
  url_loader_factory_.AddResponse("https://registry.npmjs.org/test-snap/1.0.0",
                                  MakeNpmRegistryMetadataJson(tarball_url));
  url_loader_factory_.AddResponse(tarball_url, "", net::HTTP_NOT_FOUND);
  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Failed to download snap tarball");
}

TEST_F(SnapInstallerTest, CorruptTarballFails) {
  const std::string tarball_url =
      "https://registry.npmjs.org/test-snap/-/pkg-1.0.0.tgz";
  url_loader_factory_.AddResponse("https://registry.npmjs.org/test-snap/1.0.0",
                                  MakeNpmRegistryMetadataJson(tarball_url));
  url_loader_factory_.AddResponse(tarball_url, "not-a-gzip-tarball");
  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Failed to decompress tarball");
}

TEST_F(SnapInstallerTest, DisallowedPermissionFails) {
  const std::string bundle = "code";
  TestSnapManifestOptions options;
  options.shasum = ComputeSnapBundleShasum(bundle);
  options.permissions = {"evil:permission"};
  RegisterSnap("npm:test-snap", "1.0.0", bundle, MakeSnapManifestJson(options));

  auto result = Prepare("npm:test-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_THAT(result.error(), testing::HasSubstr("disallowed permission"));
}

TEST_F(SnapInstallerTest, PrepareRejectsOversizedBundle) {
  // kMaxBundleBytes is 20 MB; exceed it by one byte (~20 MB allocation). The
  // shasum is intentionally left as the default — a mismatch only warns, so the
  // size check is what blocks the install.
  const std::string bundle(20 * 1024 * 1024 + 1, 'a');
  RegisterSnap("npm:big-snap", "1.0.0", bundle,
               MakeSnapManifestJson(TestSnapManifestOptions()));

  auto result = Prepare("npm:big-snap", "1.0.0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Snap bundle exceeds size limit");
}

// A checksum mismatch currently only logs a warning and does NOT block the
// install (see the TODO in SnapInstaller::OnBundleExtracted). This pins that
// temporary behavior; when the hard failure is restored, update this test.
TEST_F(SnapInstallerTest, ChecksumMismatchStillSucceeds) {
  const std::string bundle = "real-bundle-bytes";
  TestSnapManifestOptions options;
  options.shasum = "deliberately-wrong-shasum";
  RegisterSnap("npm:test-snap", "1.0.0", bundle, MakeSnapManifestJson(options));

  auto result = Prepare("npm:test-snap", "1.0.0");
  EXPECT_TRUE(result.has_value()) << result.error();
}

TEST_F(SnapInstallerTest, UninstallSnapRemovesFromRegistry) {
  RegisterValidSnap("npm:test-snap", "1.0.0");
  ASSERT_TRUE(Prepare("npm:test-snap", "1.0.0").has_value());
  ASSERT_TRUE(Finish("npm:test-snap").has_value());
  ASSERT_TRUE(IsInstalled("npm:test-snap"));

  installer_->UninstallSnap("npm:test-snap");

  EXPECT_FALSE(IsInstalled("npm:test-snap"));
  task_environment_.RunUntilIdle();  // Flush the async bundle deletion.
}

}  // namespace brave_wallet
