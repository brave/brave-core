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
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/snap/snap_registry.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class SnapStorage;

// Lightweight metadata for an installed snap stored in PrefService.
struct InstalledSnapInfo {
  InstalledSnapInfo();
  InstalledSnapInfo(const InstalledSnapInfo&);
  InstalledSnapInfo& operator=(const InstalledSnapInfo&);
  InstalledSnapInfo(InstalledSnapInfo&&);
  InstalledSnapInfo& operator=(InstalledSnapInfo&&);
  ~InstalledSnapInfo();

  std::string snap_id;
  std::string version;
  std::string proposed_name;
  std::vector<std::string> permissions;
  SnapRpcEndowment endowment_rpc;
  bool enabled = true;
};

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
//   6. PersistMetadata — write lightweight info to PrefService
//
// At most one concurrent install per snap_id is allowed; calling InstallSnap
// for an already-installing snap_id is a no-op (the new callback is dropped).
class SnapInstaller {
 public:
  // Plain C++ result returned by PrepareInstall so snap_installer.h stays
  // free of the mojom header.  BraveWalletService converts this to
  // mojom::SnapInstallManifestPtr before storing it.
  struct PrepareResult {
    PrepareResult();
    PrepareResult(const PrepareResult&);
    PrepareResult& operator=(const PrepareResult&);
    PrepareResult(PrepareResult&&);
    PrepareResult& operator=(PrepareResult&&);
    ~PrepareResult();

    std::string snap_id;
    std::string version;
    std::string proposed_name;
    std::string description;
    uint64_t bundle_size_bytes = 0;
    std::vector<std::string> permissions;
    std::string manifest_json;
    std::string icon_svg;
    std::string error;  // non-empty on failure
  };

  using InstallCallback =
      base::OnceCallback<void(bool success, const std::string& error)>;
  using PrepareCallback = base::OnceCallback<void(PrepareResult result)>;

  SnapInstaller(
      PrefService* prefs,
      SnapStorage* storage,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SnapInstaller();

  SnapInstaller(const SnapInstaller&) = delete;
  SnapInstaller& operator=(const SnapInstaller&) = delete;

  // Registers the pref key used to persist installed snap metadata.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Two-phase install API used for the UI approval flow:
  //
  // Phase 1 — download tarball, extract, validate. No file writes.
  // On success |callback| receives a populated SnapInstallManifest and an
  // empty error string; the prepared context is held internally keyed by
  // snap_id. On failure |manifest| is null and |error| is non-empty.
  void PrepareInstall(const std::string& snap_id,
                      const std::string& version,
                      PrepareCallback callback);

  // Phase 2 — persist the prepared bundle to disk and update PrefService.
  // Must be called after PrepareInstall succeeds for the same snap_id.
  void FinishInstall(const std::string& snap_id, InstallCallback callback);

  // Drops the prepared state without installing. No-op if snap_id is not
  // currently prepared.
  void AbortInstall(const std::string& snap_id);

  // Convenience: chains PrepareInstall → FinishInstall. Used by developer
  // flows (dev-cosmos, dev-filecoin, dev-polkadot) and EthereumProviderImpl.
  // If the snap is already installed, |callback| is invoked synchronously with
  // success=true.
  void InstallSnap(const std::string& snap_id,
                   const std::string& version,
                   InstallCallback callback);

  // Removes snap metadata from PrefService and deletes its files from disk.
  void UninstallSnap(const std::string& snap_id);

  // Reads bundle.js from disk for |snap_id|. Async; returns nullopt on error.
  void GetSnapBundle(
      const std::string& snap_id,
      base::OnceCallback<void(std::optional<std::string>)> cb);

  // Synchronous accessors backed by PrefService (fast, no I/O).
  bool IsInstalled(const std::string& snap_id) const;
  std::optional<InstalledSnapInfo> GetInstalledSnap(
      const std::string& snap_id) const;
  std::vector<InstalledSnapInfo> GetInstalledSnaps() const;

 private:
  // Per-install context carried through the async pipeline.
  struct InstallContext {
    InstallContext();
    ~InstallContext();

    std::string snap_id;
    std::string version;
    std::string proposed_name;
    GURL tarball_url;
    std::vector<std::string> permissions;
    SnapRpcEndowment endowment_rpc;
    std::string bundle_shasum;   // expected SHA-256 (base64) from snap manifest
    std::string manifest_json;   // snap.manifest.json — saved to disk
    std::string bundle_js;       // JS bundle — saved to disk
    // Set for the two-phase flow (PrepareInstall/FinishInstall); null for the
    // one-shot InstallSnap wrapper.
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
  void OnBundleExtracted(std::unique_ptr<InstallContext> ctx,
                         std::string manifest_json,
                         std::string bundle_js,
                         std::string computed_shasum,
                         std::string error);
  void OnBundleSaved(std::unique_ptr<InstallContext> ctx, bool success);

  // Completes |ctx|'s pipeline with an error.
  void FailInstall(std::unique_ptr<InstallContext> ctx,
                   const std::string& error);

  raw_ptr<PrefService> prefs_;
  raw_ptr<SnapStorage> storage_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Prepared (downloaded + validated, not yet persisted) contexts keyed by
  // snap_id, held between PrepareInstall and FinishInstall/AbortInstall.
  std::map<std::string, std::unique_ptr<InstallContext>> prepared_installs_;

  base::WeakPtrFactory<SnapInstaller> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_INSTALLER_H_
