/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_TEST_UTILS_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_bridge_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

// Shared scaffolding for snap unit tests. Everything snaps tests need to
// fabricate inputs is generated here so no binary fixtures are checked in:
//   - manifest / install-data builders (JSON and mojom),
//   - in-memory ustar tar + gzip generation for the install pipeline,
//   - npm-registry metadata builder for SnapInstaller URL mocking,
//   - FakeSnapBridge (the mojom::SnapBridge renderer interface),
//   - FakeSnapBridgeController (the C++ SnapBridgeController abstraction).
namespace brave_wallet {

// ---------------------------------------------------------------------------
// Manifest / install-data builders
// ---------------------------------------------------------------------------

// Options for MakeSnapManifestJson. Defaults produce a minimal valid manifest
// with a single snap_dialog permission and no endowment:rpc block.
struct TestSnapManifestOptions {
  TestSnapManifestOptions();
  TestSnapManifestOptions(const TestSnapManifestOptions&);
  TestSnapManifestOptions& operator=(const TestSnapManifestOptions&);
  ~TestSnapManifestOptions();

  std::string proposed_name = "Test Snap";
  std::string description = "A snap used in tests";
  // source.shasum. Set to ComputeSnapBundleShasum(bundle) for the install
  // pipeline to validate; leave as-is (or set a bogus value) to exercise the
  // shasum-mismatch path.
  std::string shasum = "test-shasum";
  // source.location.npm.filePath — the bundle path the decompressor looks for.
  std::string bundle_file_path = "dist/bundle.js";
  std::vector<std::string> permissions = {"snap_dialog"};
  // When true, an endowment:rpc block is emitted (and "endowment:rpc" is added
  // to the permission set) using the allow_* / allowed_rpc_origins fields.
  bool include_rpc_endowment = false;
  bool allow_dapps = false;
  bool allow_snaps = false;
  std::vector<std::string> allowed_rpc_origins;
};

// Serializes a snap.manifest.json string honoring |options| exactly.
std::string MakeSnapManifestJson(const TestSnapManifestOptions& options);

// Returns base64(sha256(bundle_js)) — the source.shasum a source-only snap
// must declare to pass SnapInstaller validation. Mirrors
// SnapInstallerChecksumCalculator for the no-icon / no-aux-file case.
std::string ComputeSnapBundleShasum(const std::string& bundle_js);

// Builds an in-memory mojom::SnapManifest. Defaults align with the JSON
// produced by MakeSnapManifestJson.
mojom::SnapManifestPtr MakeTestSnapManifest(
    std::vector<std::string> permissions = {"snap_dialog"},
    bool allow_dapps = false,
    std::vector<std::string> allowed_rpc_origins = {});

// Builds mojom::SnapInstallData for registry / data-provider tests.
mojom::SnapInstallDataPtr MakeTestSnapInstallData(
    const std::string& snap_id = "npm:@test/snap",
    const std::string& version = "1.0.0");

// ---------------------------------------------------------------------------
// Tar / tarball builders
// ---------------------------------------------------------------------------

// Builds a raw POSIX ustar archive from (path, content) pairs. Each path must
// be at most 100 bytes (the ustar name field; no prefix splitting is done).
std::string BuildUstarTar(
    const std::vector<std::pair<std::string, std::string>>& entries);

// Builds the uncompressed tar bytes for a snap package containing
// "package/snap.manifest.json" and "package/<bundle_file_path>".
std::string BuildSnapTar(
    const std::string& manifest_json,
    const std::string& bundle_js,
    const std::string& bundle_file_path = "dist/bundle.js");

// gzip-compresses |data|; the install pipeline expects a .tgz on the wire.
std::string GzipCompressForTest(const std::string& data);

// Convenience: builds a gzipped snap tarball from |manifest_json| +
// |bundle_js|. Equivalent to GzipCompressForTest(BuildSnapTar(...)).
std::string BuildSnapTarball(
    const std::string& manifest_json,
    const std::string& bundle_js,
    const std::string& bundle_file_path = "dist/bundle.js");

// Builds the registry.npmjs.org metadata JSON SnapInstaller fetches.
// "dist.tarball" and top-level "version" are consumed; "dist.shasum" is
// included for realism.
std::string MakeNpmRegistryMetadataJson(const std::string& tarball_url,
                                        const std::string& shasum = "",
                                        const std::string& version = "");

// ---------------------------------------------------------------------------
// FakeSnapBridge — implements the mojom::SnapBridge renderer interface
// ---------------------------------------------------------------------------

// Used by WalletPageSnapBridgeController /
// HiddenWebContentsSnapBridgeController tests: bind it as the page-side bridge
// and assert passthrough calls. Each method records its arguments and
// immediately runs the callback with the matching canned response member.
class FakeSnapBridge : public mojom::SnapBridge {
 public:
  FakeSnapBridge();
  ~FakeSnapBridge() override;

  FakeSnapBridge(const FakeSnapBridge&) = delete;
  FakeSnapBridge& operator=(const FakeSnapBridge&) = delete;

  // Binds a fresh pipe and returns the remote end for SetBridge().
  mojo::PendingRemote<mojom::SnapBridge> BindNewPipeAndPassRemote();
  void Reset();

  // Canned responses (mutate before the call under test).
  bool load_snap_success = true;
  std::optional<std::string> load_snap_error;
  std::optional<base::Value> invoke_result;
  std::optional<std::string> invoke_error;
  std::optional<std::string> home_page_content_json = "{}";
  std::optional<std::string> home_page_interface_id = "iface-1";
  std::optional<std::string> home_page_error;
  std::optional<std::string> user_input_content_json = "{}";
  std::optional<std::string> user_input_error;

  // Recorded calls.
  int load_snap_call_count = 0;
  int invoke_snap_call_count = 0;
  int unload_snap_call_count = 0;
  int fetch_home_page_call_count = 0;
  int user_input_call_count = 0;
  std::string last_snap_id;
  std::string last_method;
  base::Value last_params;
  std::string last_caller_origin;
  std::string last_interface_id;
  std::string last_event_json;

  // mojom::SnapBridge:
  void LoadSnap(const std::string& snap_id, LoadSnapCallback cb) override;
  void InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  base::Value params,
                  const std::string& caller_origin,
                  InvokeSnapCallback cb) override;
  void UnloadSnap(const std::string& snap_id, UnloadSnapCallback cb) override;
  void FetchSnapHomePage(const std::string& snap_id,
                         FetchSnapHomePageCallback cb) override;
  void SendSnapUserInputEvent(const std::string& snap_id,
                              const std::string& interface_id,
                              const std::string& event_json,
                              SendSnapUserInputEventCallback cb) override;

 private:
  mojo::Receiver<mojom::SnapBridge> receiver_{this};
};

// ---------------------------------------------------------------------------
// FakeSnapBridgeController — implements the C++ SnapBridgeController
// ---------------------------------------------------------------------------

// Used by SnapController tests. By default every async method runs its callback
// synchronously with the matching canned response. Set the auto_run_* flags to
// false to capture callbacks and fire them later via the Run*/Fire* helpers —
// needed to assert LoadSnap→InvokeSnap sequencing and disconnect handling.
class FakeSnapBridgeController : public SnapBridgeController {
 public:
  FakeSnapBridgeController();
  ~FakeSnapBridgeController() override;

  FakeSnapBridgeController(const FakeSnapBridgeController&) = delete;
  FakeSnapBridgeController& operator=(const FakeSnapBridgeController&) = delete;

  struct InvokeCall {
    InvokeCall();
    InvokeCall(InvokeCall&&);
    InvokeCall& operator=(InvokeCall&&);
    ~InvokeCall();

    std::string snap_id;
    std::string method;
    base::Value params;
    std::string caller_origin;
  };

  // Config.
  bool is_bound = true;
  bool auto_run_ensure_ready = true;
  bool auto_run_load = true;
  bool load_success = true;
  std::optional<std::string> load_error;
  bool auto_run_invoke = true;
  std::optional<base::Value> invoke_result;
  std::optional<std::string> invoke_error;
  std::optional<std::string> home_page_content_json = "{}";
  std::optional<std::string> home_page_interface_id = "iface-1";
  std::optional<std::string> home_page_error;
  std::optional<std::string> user_input_content_json = "{}";
  std::optional<std::string> user_input_error;

  // Recorded calls.
  int set_bridge_call_count = 0;
  int ensure_ready_call_count = 0;
  std::vector<std::string> load_snap_ids;
  std::vector<InvokeCall> invoke_calls;

  // Manual-firing helpers (used when the matching auto_run_* flag is false).
  bool HasPendingReady() const { return !pending_ready_cb_.is_null(); }
  void RunPendingReady();
  bool HasPendingLoad() const { return !pending_load_cb_.is_null(); }
  void RunPendingLoad(bool success, std::optional<std::string> error);
  bool HasPendingInvoke() const { return !pending_invoke_cb_.is_null(); }
  void RunPendingInvoke(std::optional<base::Value> result,
                        std::optional<std::string> error);
  // Invokes the registered disconnect callback (simulates the bridge dropping).
  void FireDisconnect();

  // SnapBridgeController:
  void SetBridge(mojo::PendingRemote<mojom::SnapBridge> bridge) override;
  bool IsBound() const override;
  void SetDisconnectCallback(DisconnectCallback cb) override;
  void EnsureBridgeReady(base::OnceClosure on_ready) override;
  void LoadSnap(const std::string& snap_id, LoadSnapCallback cb) override;
  void InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  base::Value params,
                  const std::string& caller_origin,
                  InvokeSnapCallback cb) override;
  void FetchSnapHomePage(const std::string& snap_id,
                         FetchSnapHomePageCallback cb) override;
  void SendSnapUserInputEvent(const std::string& snap_id,
                              const std::string& interface_id,
                              const std::string& event_json,
                              SendSnapUserInputEventCallback cb) override;

 private:
  DisconnectCallback disconnect_cb_;
  base::OnceClosure pending_ready_cb_;
  LoadSnapCallback pending_load_cb_;
  InvokeSnapCallback pending_invoke_cb_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_TEST_UTILS_H_
