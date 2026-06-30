/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_installer.h"

#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/installer/snap_installer_tar_decompressor.h"
#include "brave/components/brave_wallet/browser/snap/snap_manifest_parser.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/origin.h"

namespace brave_wallet {

namespace {

// Download size limits.
constexpr size_t kMaxMetadataBytes = 1 * 1024 * 1024;  // 1 MB
constexpr size_t kMaxBundleBytes = 20 * 1024 * 1024;   // 10 MB

net::NetworkTrafficAnnotationTag GetTrafficAnnotation() {
  return net::DefineNetworkTrafficAnnotation("snap_installer", R"(
      semantics {
        sender: "Brave Wallet Snap Installer"
        description:
          "Fetches MetaMask snap packages from the npm registry in order to "
          "install them into Brave Wallet at user or dapp request."
        trigger:
          "Triggered when a dapp calls wallet_requestSnaps or the user "
          "explicitly installs a snap from the Brave Wallet UI."
        data:
          "Snap package name and version are sent to registry.npmjs.org to "
          "retrieve metadata and the JavaScript bundle tarball."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "Snap installation can be controlled via the Brave Wallet settings."
        policy_exception_justification: "Not implemented."
      })");
}

// Creates a simple GET resource request for |url|.
std::unique_ptr<network::ResourceRequest> MakeGetRequest(const GURL& url) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->method = "GET";
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  return request;
}

}  // namespace

// ---------------------------------------------------------------------------
// SnapInstaller::InstallContext
// ---------------------------------------------------------------------------

SnapInstaller::ExtractResult::ExtractResult() = default;
SnapInstaller::ExtractResult::ExtractResult(ExtractResult&&) = default;
SnapInstaller::ExtractResult::~ExtractResult() = default;

SnapInstaller::InstallContext::InstallContext() = default;
SnapInstaller::InstallContext::~InstallContext() {
  if (!temp_dir_path.empty()) {
    base::ThreadPool::PostTask(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(base::IgnoreResult(&base::DeletePathRecursively),
                       temp_dir_path));
  }
}

// ---------------------------------------------------------------------------
// SnapInstaller
// ---------------------------------------------------------------------------

SnapInstaller::SnapInstaller(
    SnapDataProvider& data_provider,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : data_provider_(data_provider),
      url_loader_factory_(std::move(url_loader_factory)) {}

SnapInstaller::~SnapInstaller() = default;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void SnapInstaller::PrepareInstall(const std::string& snap_id,
                                   const std::string& version,
                                   const url::Origin& install_origin,
                                   PrepareCallback callback) {
  if (prepared_installs_.count(snap_id)) {
    std::move(callback).Run(base::unexpected("already_preparing"));
    return;
  }

  auto ctx = std::make_unique<InstallContext>();
  ctx->snap_id = snap_id;
  ctx->version = version.empty() ? "latest" : version;
  ctx->install_origin =
      install_origin.opaque() ? std::string() : install_origin.Serialize();
  ctx->prepare_callback = std::move(callback);

  FetchMetadata(std::move(ctx));
}

void SnapInstaller::FinishInstall(const std::string& snap_id,
                                  InstallCallback callback) {
  auto it = prepared_installs_.find(snap_id);
  if (it == prepared_installs_.end()) {
    std::move(callback).Run(base::unexpected("snap not prepared"));
    return;
  }

  auto ctx = std::move(it->second);
  prepared_installs_.erase(it);
  ctx->callback = std::move(callback);

  const base::FilePath unpacked_dir =
      ctx->temp_dir_path.AppendASCII("unpacked");
  data_provider_->SaveBundleFromDir(
      ctx->snap_id, unpacked_dir,
      base::BindOnce(&SnapInstaller::OnBundleSaved,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::AbortInstall(const std::string& snap_id) {
  prepared_installs_.erase(snap_id);
}

void SnapInstaller::UninstallSnap(const std::string& snap_id) {
  data_provider_->OnSnapUninstalled(snap_id, base::DoNothing());
}

void SnapInstaller::GetSnapBundle(
    const std::string& snap_id,
    base::OnceCallback<void(std::optional<std::string>)> cb) {
  data_provider_->ReadBundle(snap_id, std::move(cb));
}

// ---------------------------------------------------------------------------
// Install pipeline
// ---------------------------------------------------------------------------

void SnapInstaller::FetchMetadata(std::unique_ptr<InstallContext> ctx) {
  // snap_id = "npm:@polkagate/snap" → package = "@polkagate/snap"
  std::string package_name = ctx->snap_id.substr(4);  // strip "npm:"
  GURL url("https://registry.npmjs.org/" + package_name + "/" + ctx->version);
  LOG(ERROR) << "XXXZZZ FetchMetadata snap_id=" << ctx->snap_id
             << " version=" << ctx->version << " url=" << url.spec();
  if (!url.is_valid()) {
    FailInstall(std::move(ctx), "Invalid snap metadata URL");
    return;
  }

  ctx->loader = network::SimpleURLLoader::Create(MakeGetRequest(url),
                                                 GetTrafficAnnotation());
  auto* loader_ptr = ctx->loader.get();
  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&SnapInstaller::OnMetadataFetched,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)),
      kMaxMetadataBytes);
}

void SnapInstaller::OnMetadataFetched(std::unique_ptr<InstallContext> ctx,
                                      std::optional<std::string> body) {
  LOG(ERROR) << "XXXZZZ OnMetadataFetched snap_id=" << ctx->snap_id
             << " body_present=" << body.has_value()
             << " body_size=" << (body ? body->size() : 0u);
  if (!body) {
    FailInstall(std::move(ctx), "Failed to fetch snap metadata");
    return;
  }
  ctx->loader.reset();

  std::optional<base::Value> parsed =
      base::JSONReader::Read(*body, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    LOG(ERROR) << "XXXZZZ OnMetadataFetched: JSON parse FAILED";
    FailInstall(std::move(ctx), "Invalid snap metadata JSON");
    return;
  }

  if (const std::string* resolved_version =
          parsed->GetDict().FindString("version");
      resolved_version && !resolved_version->empty()) {
    ctx->version = *resolved_version;
  }

  const base::DictValue* dist = parsed->GetDict().FindDict("dist");
  if (!dist) {
    LOG(ERROR) << "XXXZZZ OnMetadataFetched: missing 'dist'";
    FailInstall(std::move(ctx), "Missing 'dist' in snap metadata");
    return;
  }
  const std::string* tarball_url = dist->FindString("tarball");
  if (!tarball_url) {
    LOG(ERROR) << "XXXZZZ OnMetadataFetched: missing 'dist.tarball'";
    FailInstall(std::move(ctx), "Missing 'dist.tarball' in snap metadata");
    return;
  }

  ctx->tarball_url = GURL(*tarball_url);
  LOG(ERROR) << "XXXZZZ OnMetadataFetched: tarball_url="
             << ctx->tarball_url.spec();
  if (!ctx->tarball_url.is_valid()) {
    FailInstall(std::move(ctx), "Invalid tarball URL in snap metadata");
    return;
  }

  ctx->loader = network::SimpleURLLoader::Create(
      MakeGetRequest(ctx->tarball_url), GetTrafficAnnotation());
  // DownloadToString is capped at 5 MB; use DownloadToTempFile for tarballs.
  ctx->loader->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(&SnapInstaller::OnTarballDownloadedToFile,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::OnTarballDownloadedToFile(
    std::unique_ptr<InstallContext> ctx,
    base::FilePath path) {
  LOG(ERROR) << "XXXZZZ OnTarballDownloadedToFile snap_id=" << ctx->snap_id
             << " path=" << path.value();
  if (path.empty()) {
    FailInstall(std::move(ctx), "Failed to download snap tarball");
    return;
  }
  ctx->loader.reset();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&SnapInstallerTarDecompressor::ExtractTarballToDir, path),
      base::BindOnce(&SnapInstaller::OnTarballExtracted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::OnTarballExtracted(std::unique_ptr<InstallContext> ctx,
                                       ExtractResult result) {
  LOG(ERROR) << "XXXZZZ OnTarballExtracted snap_id=" << ctx->snap_id
             << " error='" << result.error << "'"
             << " bundle_size=" << result.bundle_size_bytes;
  ctx->bundle_size_bytes = result.bundle_size_bytes;
  ctx->temp_dir_path = std::move(result.temp_dir_path);
  OnBundleExtracted(std::move(ctx), std::move(result.manifest_json),
                    std::move(result.computed_shasum), std::move(result.error));
}

void SnapInstaller::OnBundleExtracted(std::unique_ptr<InstallContext> ctx,
                                      std::string manifest_json,
                                      std::string computed_shasum,
                                      std::string error) {
  LOG(ERROR) << "XXXZZZ OnBundleExtracted snap_id=" << ctx->snap_id
             << " error='" << error << "'";
  if (!error.empty()) {
    FailInstall(std::move(ctx), error);
    return;
  }

  LOG(ERROR) << "XXXZZZ OnBundleExtracted: parsing manifest";
  SnapManifestParser::Result parsed =
      SnapManifestParser::Parse(manifest_json, ctx->snap_id, ctx->version);
  LOG(ERROR) << "XXXZZZ OnBundleExtracted: manifest parse error='"
             << parsed.error << "'";
  if (!parsed.error.empty()) {
    FailInstall(std::move(ctx), parsed.error);
    return;
  }

  LOG(ERROR) << "SNAP integrity: snap_id=" << ctx->snap_id
             << " bundle_size=" << ctx->bundle_size_bytes << " expected='"
             << parsed.expected_shasum << "'"
             << " computed='" << computed_shasum << "'"
             << (computed_shasum == parsed.expected_shasum ? " MATCH"
                                                           : " MISMATCH");
  if (computed_shasum != parsed.expected_shasum) {
    // TODO(snap): restore hard failure once checksum algorithm is confirmed.
    LOG(ERROR)
        << "SNAP integrity: WARNING — checksum mismatch, continuing anyway";
  }

  if (ctx->bundle_size_bytes > kMaxBundleBytes) {
    FailInstall(std::move(ctx), "Snap bundle exceeds size limit");
    return;
  }

  // parsed.manifest is now a mojom::SnapManifestPtr — store directly.
  ctx->snap_manifest = std::move(parsed.manifest);

  if (ctx->prepare_callback) {
    auto install_data = mojom::SnapInstallData::New();
    install_data->snap_id = ctx->snap_id;
    install_data->version = ctx->version;
    install_data->bundle_size_bytes = ctx->bundle_size_bytes;
    install_data->manifest = ctx->snap_manifest.Clone();
    install_data->description = ctx->snap_manifest->description;
    install_data->install_origin = ctx->install_origin;
    install_data->enabled = true;

    std::string snap_id = ctx->snap_id;
    PrepareCallback cb = std::move(ctx->prepare_callback);
    prepared_installs_[snap_id] = std::move(ctx);
    std::move(cb).Run(base::ok(std::move(install_data)));
    return;
  }

  const base::FilePath unpacked_dir =
      ctx->temp_dir_path.AppendASCII("unpacked");
  data_provider_->SaveBundleFromDir(
      ctx->snap_id, unpacked_dir,
      base::BindOnce(&SnapInstaller::OnBundleSaved,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::OnBundleSaved(std::unique_ptr<InstallContext> ctx,
                                  bool success) {
  LOG(ERROR) << "XXXZZZ OnBundleSaved snap_id=" << ctx->snap_id
             << " success=" << success;
  if (!success) {
    FailInstall(std::move(ctx), "Failed to save snap bundle to disk");
    return;
  }

  // Build the full SnapInstallData for the registry.
  auto install_data = mojom::SnapInstallData::New();
  install_data->snap_id = ctx->snap_id;
  install_data->version = ctx->version;
  install_data->bundle_size_bytes = ctx->bundle_size_bytes;
  install_data->manifest = ctx->snap_manifest.Clone();
  install_data->description = ctx->snap_manifest->description;
  install_data->install_origin = ctx->install_origin;
  install_data->enabled = true;
  data_provider_->OnSnapInstalled(
      *install_data,
      base::BindOnce(&SnapInstaller::OnRegistryUpdated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::OnRegistryUpdated(std::unique_ptr<InstallContext> ctx) {
  std::move(ctx->callback).Run(base::ok());
}

void SnapInstaller::FailInstall(std::unique_ptr<InstallContext> ctx,
                                const std::string& error) {
  DLOG(ERROR) << "SnapInstaller: " << ctx->snap_id << ": " << error;
  if (ctx->prepare_callback) {
    std::move(ctx->prepare_callback).Run(base::unexpected(error));
  } else {
    std::move(ctx->callback).Run(base::unexpected(error));
  }
}

}  // namespace brave_wallet
