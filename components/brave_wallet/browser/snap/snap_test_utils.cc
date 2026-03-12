/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/json/json_writer.h"
#include "crypto/hash.h"
#include "third_party/zlib/google/compression_utils.h"

namespace brave_wallet {

namespace {

// POSIX ustar header layout (must match snap_tar_utils.cc).
constexpr size_t kBlockSize = 512;

// Writes |value| as (width - 1) zero-padded octal digits followed by a NUL
// terminator into |buf| starting at |offset|.
void SetOctalField(std::string& buf,
                   size_t offset,
                   size_t width,
                   uint64_t value) {
  std::string oct;
  do {
    oct.push_back(static_cast<char>('0' + (value & 7)));
    value >>= 3;
  } while (value != 0);
  std::ranges::reverse(oct);

  const size_t digits = width - 1;
  CHECK_LE(oct.size(), digits);
  std::string field(digits - oct.size(), '0');
  field += oct;
  for (size_t i = 0; i < digits; ++i) {
    buf[offset + i] = field[i];
  }
  buf[offset + width - 1] = '\0';
}

// Computes the ustar header checksum over |header| (with the checksum field
// treated as spaces) and writes it back as "NNNNNN\0 ".
void SetChecksumField(std::string& header) {
  for (size_t i = 148; i < 156; ++i) {
    header[i] = ' ';
  }
  uint32_t sum = 0;
  for (char c : header) {
    sum += static_cast<unsigned char>(c);
  }
  std::string oct;
  do {
    oct.push_back(static_cast<char>('0' + (sum & 7)));
    sum >>= 3;
  } while (sum != 0);
  std::ranges::reverse(oct);

  CHECK_LE(oct.size(), 6u);
  std::string field(6 - oct.size(), '0');
  field += oct;
  for (size_t i = 0; i < 6; ++i) {
    header[148 + i] = field[i];
  }
  header[154] = '\0';
  header[155] = ' ';
}

}  // namespace

// ---------------------------------------------------------------------------
// Manifest / install-data builders
// ---------------------------------------------------------------------------

TestSnapManifestOptions::TestSnapManifestOptions() = default;
TestSnapManifestOptions::TestSnapManifestOptions(const TestSnapManifestOptions&) =
    default;
TestSnapManifestOptions& TestSnapManifestOptions::operator=(
    const TestSnapManifestOptions&) = default;
TestSnapManifestOptions::~TestSnapManifestOptions() = default;

std::string MakeSnapManifestJson(const TestSnapManifestOptions& options) {
  base::DictValue initial_perms;
  for (const auto& perm : options.permissions) {
    if (perm == "endowment:rpc") {
      continue;  // Emitted below with its config block.
    }
    initial_perms.Set(perm, base::DictValue());
  }
  if (options.include_rpc_endowment) {
    base::DictValue rpc;
    rpc.Set("dapps", options.allow_dapps);
    rpc.Set("snaps", options.allow_snaps);
    if (!options.allowed_rpc_origins.empty()) {
      base::ListValue origins;
      for (const auto& origin : options.allowed_rpc_origins) {
        origins.Append(origin);
      }
      rpc.Set("allowedOrigins", std::move(origins));
    }
    initial_perms.Set("endowment:rpc", std::move(rpc));
  }

  base::DictValue npm;
  npm.Set("filePath", options.bundle_file_path);
  base::DictValue location;
  location.Set("npm", std::move(npm));
  base::DictValue source;
  source.Set("shasum", options.shasum);
  source.Set("location", std::move(location));

  base::DictValue manifest;
  manifest.Set("proposedName", options.proposed_name);
  manifest.Set("description", options.description);
  manifest.Set("source", std::move(source));
  manifest.Set("initialPermissions", std::move(initial_perms));
  return base::WriteJson(manifest).value_or("{}");
}

std::string ComputeSnapBundleShasum(const std::string& bundle_js) {
  return base::Base64Encode(crypto::hash::Sha256(bundle_js));
}

mojom::SnapManifestPtr MakeTestSnapManifest(
    std::vector<std::string> permissions,
    bool allow_dapps,
    std::vector<std::string> allowed_rpc_origins) {
  auto manifest = mojom::SnapManifest::New();
  manifest->proposed_name = "Test Snap";
  manifest->description = "A snap used in tests";
  manifest->permissions = std::move(permissions);
  manifest->allow_dapps = allow_dapps;
  manifest->allow_snaps = false;
  manifest->allowed_rpc_origins = std::move(allowed_rpc_origins);
  return manifest;
}

mojom::SnapInstallDataPtr MakeTestSnapInstallData(const std::string& snap_id,
                                                  const std::string& version) {
  auto data = mojom::SnapInstallData::New();
  data->snap_id = snap_id;
  data->version = version;
  data->bundle_size_bytes = 1024;
  data->icon_svg = "";
  data->manifest = MakeTestSnapManifest();
  data->enabled = true;
  return data;
}

// ---------------------------------------------------------------------------
// Tar / tarball builders
// ---------------------------------------------------------------------------

std::string BuildUstarTar(
    const std::vector<std::pair<std::string, std::string>>& entries) {
  std::string out;
  for (const auto& [path, content] : entries) {
    CHECK_LE(path.size(), 100u) << "ustar name field overflow: " << path;
    std::string header(kBlockSize, '\0');
    std::ranges::copy(path, header.begin());           // name @ 0
    SetOctalField(header, 100, 8, 0644);               // mode
    SetOctalField(header, 108, 8, 0);                  // uid
    SetOctalField(header, 116, 8, 0);                  // gid
    SetOctalField(header, 124, 12, content.size());    // size
    SetOctalField(header, 136, 12, 0);                 // mtime
    header[156] = '0';                                 // typeflag: regular file
    std::ranges::copy(std::string_view("ustar"),       // magic @ 257
                      header.begin() + 257);
    header[263] = '0';                                 // version @ 263 ("00")
    header[264] = '0';
    SetChecksumField(header);
    out += header;

    out += content;
    const size_t remainder = content.size() % kBlockSize;
    if (remainder != 0) {
      out.append(kBlockSize - remainder, '\0');
    }
  }
  out.append(kBlockSize * 2, '\0');  // Two zero blocks: end-of-archive marker.
  return out;
}

std::string BuildSnapTar(const std::string& manifest_json,
                         const std::string& bundle_js,
                         const std::string& bundle_file_path) {
  return BuildUstarTar({
      {"package/snap.manifest.json", manifest_json},
      {"package/" + bundle_file_path, bundle_js},
  });
}

std::string GzipCompressForTest(const std::string& data) {
  std::string compressed;
  CHECK(compression::GzipCompress(base::as_byte_span(data), &compressed));
  return compressed;
}

std::string BuildSnapTarball(const std::string& manifest_json,
                             const std::string& bundle_js,
                             const std::string& bundle_file_path) {
  return GzipCompressForTest(
      BuildSnapTar(manifest_json, bundle_js, bundle_file_path));
}

std::string MakeNpmRegistryMetadataJson(const std::string& tarball_url,
                                        const std::string& shasum) {
  base::DictValue dist;
  dist.Set("tarball", tarball_url);
  if (!shasum.empty()) {
    dist.Set("shasum", shasum);
  }
  base::DictValue root;
  root.Set("dist", std::move(dist));
  return base::WriteJson(root).value_or("{}");
}

// ---------------------------------------------------------------------------
// FakeSnapBridge
// ---------------------------------------------------------------------------

FakeSnapBridge::FakeSnapBridge() {
  invoke_result = base::Value("ok");
}
FakeSnapBridge::~FakeSnapBridge() = default;

mojo::PendingRemote<mojom::SnapBridge>
FakeSnapBridge::BindNewPipeAndPassRemote() {
  return receiver_.BindNewPipeAndPassRemote();
}

void FakeSnapBridge::Reset() {
  receiver_.reset();
}

void FakeSnapBridge::LoadSnap(const std::string& snap_id, LoadSnapCallback cb) {
  ++load_snap_call_count;
  last_snap_id = snap_id;
  std::move(cb).Run(load_snap_success, load_snap_error);
}

void FakeSnapBridge::InvokeSnap(const std::string& snap_id,
                                const std::string& method,
                                base::Value params,
                                const std::string& caller_origin,
                                InvokeSnapCallback cb) {
  ++invoke_snap_call_count;
  last_snap_id = snap_id;
  last_method = method;
  last_params = std::move(params);
  last_caller_origin = caller_origin;
  std::move(cb).Run(
      invoke_result ? std::optional<base::Value>(invoke_result->Clone())
                    : std::nullopt,
      invoke_error);
}

void FakeSnapBridge::UnloadSnap(const std::string& snap_id,
                                UnloadSnapCallback cb) {
  ++unload_snap_call_count;
  last_snap_id = snap_id;
  std::move(cb).Run();
}

void FakeSnapBridge::FetchSnapHomePage(const std::string& snap_id,
                                       FetchSnapHomePageCallback cb) {
  ++fetch_home_page_call_count;
  last_snap_id = snap_id;
  std::move(cb).Run(home_page_content_json, home_page_interface_id,
                    home_page_error);
}

void FakeSnapBridge::SendSnapUserInputEvent(const std::string& snap_id,
                                            const std::string& interface_id,
                                            const std::string& event_json,
                                            SendSnapUserInputEventCallback cb) {
  ++user_input_call_count;
  last_snap_id = snap_id;
  last_interface_id = interface_id;
  last_event_json = event_json;
  std::move(cb).Run(user_input_content_json, user_input_error);
}

// ---------------------------------------------------------------------------
// FakeSnapBridgeController
// ---------------------------------------------------------------------------

FakeSnapBridgeController::InvokeCall::InvokeCall() = default;
FakeSnapBridgeController::InvokeCall::InvokeCall(InvokeCall&&) = default;
FakeSnapBridgeController::InvokeCall&
FakeSnapBridgeController::InvokeCall::operator=(InvokeCall&&) = default;
FakeSnapBridgeController::InvokeCall::~InvokeCall() = default;

FakeSnapBridgeController::FakeSnapBridgeController() {
  invoke_result = base::Value("ok");
}
FakeSnapBridgeController::~FakeSnapBridgeController() = default;

void FakeSnapBridgeController::RunPendingReady() {
  CHECK(pending_ready_cb_);
  std::move(pending_ready_cb_).Run();
}

void FakeSnapBridgeController::RunPendingLoad(bool success,
                                             std::optional<std::string> error) {
  CHECK(pending_load_cb_);
  std::move(pending_load_cb_).Run(success, error);
}

void FakeSnapBridgeController::RunPendingInvoke(
    std::optional<base::Value> result,
    std::optional<std::string> error) {
  CHECK(pending_invoke_cb_);
  std::move(pending_invoke_cb_).Run(std::move(result), error);
}

void FakeSnapBridgeController::FireDisconnect() {
  if (disconnect_cb_) {
    disconnect_cb_.Run();
  }
}

void FakeSnapBridgeController::SetBridge(
    mojo::PendingRemote<mojom::SnapBridge> bridge) {
  ++set_bridge_call_count;
}

bool FakeSnapBridgeController::IsBound() const {
  return is_bound;
}

void FakeSnapBridgeController::SetDisconnectCallback(DisconnectCallback cb) {
  disconnect_cb_ = std::move(cb);
}

void FakeSnapBridgeController::EnsureBridgeReady(base::OnceClosure on_ready) {
  ++ensure_ready_call_count;
  if (auto_run_ensure_ready) {
    std::move(on_ready).Run();
  } else {
    pending_ready_cb_ = std::move(on_ready);
  }
}

void FakeSnapBridgeController::LoadSnap(const std::string& snap_id,
                                       LoadSnapCallback cb) {
  load_snap_ids.push_back(snap_id);
  if (auto_run_load) {
    std::move(cb).Run(load_success, load_error);
  } else {
    pending_load_cb_ = std::move(cb);
  }
}

void FakeSnapBridgeController::InvokeSnap(const std::string& snap_id,
                                         const std::string& method,
                                         base::Value params,
                                         const std::string& caller_origin,
                                         InvokeSnapCallback cb) {
  InvokeCall call;
  call.snap_id = snap_id;
  call.method = method;
  call.params = std::move(params);
  call.caller_origin = caller_origin;
  invoke_calls.push_back(std::move(call));
  if (auto_run_invoke) {
    std::move(cb).Run(
        invoke_result ? std::optional<base::Value>(invoke_result->Clone())
                      : std::nullopt,
        invoke_error);
  } else {
    pending_invoke_cb_ = std::move(cb);
  }
}

void FakeSnapBridgeController::FetchSnapHomePage(const std::string& snap_id,
                                                FetchSnapHomePageCallback cb) {
  std::move(cb).Run(home_page_content_json, home_page_interface_id,
                    home_page_error);
}

void FakeSnapBridgeController::SendSnapUserInputEvent(
    const std::string& snap_id,
    const std::string& interface_id,
    const std::string& event_json,
    SendSnapUserInputEventCallback cb) {
  std::move(cb).Run(user_input_content_json, user_input_error);
}

}  // namespace brave_wallet
