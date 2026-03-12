/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_INSTALLER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/gurl.h"


namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class SnapDataProvider;

// SnapInstaller fetches, validates, and installs snaps from the MetaMask Snaps
// npm registry at runtime.
//
// Install pipeline (all network I/O on the current sequence, CPU work on the
// thread pool):
//
//   1. FetchMetadata   — GET registry.npmjs.org/<package>/<version>
//   2. DownloadTarball — GET the .tgz URL from the metadata JSON
//   3. Extract         — gzip decompress + tar parse (thread pool)
//   4. Validate        — shasum (SHA-256), permissions allowlist, size limit
//   5. PersistBundle   — write to SnapStorage (file I/O on thread pool)
//   6. PersistMetadata — write to PrefService via SnapRegistry
//
// At most one concurrent install per snap_id is allowed; calling InstallSnap
// for an already-installing snap_id is a no-op (the new callback is dropped).
class SnapInstaller {
 public:
  using InstallCallback =
      base::OnceCallback<void(base::expected<void, std::string>)>;
  using PrepareCallback = base::OnceCallback<void(
      base::expected<mojom::SnapInstallDataPtr, std::string>)>;

  SnapInstaller(
      SnapDataProvider& data_provider,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SnapInstaller();

  SnapInstaller(const SnapInstaller&) = delete;
  SnapInstaller& operator=(const SnapInstaller&) = delete;

  // Two-phase install API used for the UI approval flow:
  //
  // Phase 1 — download tarball, extract, validate. No file writes.
  // On success |callback| receives a populated PrepareResult (with a
  // non-null SnapManifest); the prepared context is held internally keyed by
  // snap_id. On failure |result.error| is non-empty.
  void PrepareInstall(const std::string& snap_id,
                      const std::string& version,
                      PrepareCallback callback);

  // Phase 2 — persist the prepared bundle to disk and update PrefService via
  // SnapRegistry.
  // Must be called after PrepareInstall succeeds for the same snap_id.
  void FinishInstall(const std::string& snap_id, InstallCallback callback);

  // Drops the prepared state without installing. No-op if snap_id is not
  // currently prepared.
  void AbortInstall(const std::string& snap_id);

  // Removes snap metadata from the registry and deletes files from disk.
  void UninstallSnap(const std::string& snap_id);

  // Reads bundle.js from disk for |snap_id|. Async; returns nullopt on error.
  void GetSnapBundle(
      const std::string& snap_id,
      base::OnceCallback<void(std::optional<std::string>)> cb);

  // Result of the thread-pool extraction step. Files are written to
  // |unpacked_dir| (a subfolder of the snap's ScopedTempDir).
  struct ExtractResult {
    ExtractResult();
    ExtractResult(ExtractResult&&);
    ~ExtractResult();

    std::string manifest_json;   // text of snap.manifest.json
    std::string computed_shasum;
    uint64_t bundle_size_bytes = 0;
    // Base of the snap-specific temp dir created on the thread pool.
    // The actual files are under <temp_dir_path>/unpacked/.
    base::FilePath temp_dir_path;
    std::string error;
  };

 private:

  // Per-install context carried through the async pipeline.
  struct InstallContext {
    InstallContext();
    ~InstallContext();

    std::string snap_id;
    std::string version;
    GURL tarball_url;
    mojom::SnapManifestPtr snap_manifest;  // populated by OnBundleExtracted
    uint64_t bundle_size_bytes = 0;
    // Base of the snap-specific temp dir created on the thread pool.
    // <temp_dir_path>/unpacked/ holds bundle.js and manifest.json.
    // Deleted asynchronously on InstallContext destruction.
    base::FilePath temp_dir_path;
    // Set for the two-phase flow (PrepareInstall/FinishInstall).
    PrepareCallback prepare_callback;
    InstallCallback callback;
    std::unique_ptr<network::SimpleURLLoader> loader;
  };

  // Install pipeline steps.
  void FetchMetadata(std::unique_ptr<InstallContext> ctx);
  void OnMetadataFetched(std::unique_ptr<InstallContext> ctx,
                         std::optional<std::string> body);
  // DownloadToTempFile callback — path is empty on error.
  void OnTarballDownloadedToFile(std::unique_ptr<InstallContext> ctx,
                                 base::FilePath path);
  void OnTarballExtracted(std::unique_ptr<InstallContext> ctx,
                          ExtractResult result);
  void OnBundleExtracted(std::unique_ptr<InstallContext> ctx,
                         std::string manifest_json,
                         std::string computed_shasum,
                         std::string error);
  void OnBundleSaved(std::unique_ptr<InstallContext> ctx, bool success);

  // Completes |ctx|'s pipeline with an error.
  void FailInstall(std::unique_ptr<InstallContext> ctx,
                   const std::string& error);


  raw_ref<SnapDataProvider> data_provider_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Prepared (downloaded + validated, not yet persisted) contexts keyed by
  // snap_id, held between PrepareInstall and FinishInstall/AbortInstall.
  std::map<std::string, std::unique_ptr<InstallContext>> prepared_installs_;

  base::WeakPtrFactory<SnapInstaller> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_INSTALLER_H_
