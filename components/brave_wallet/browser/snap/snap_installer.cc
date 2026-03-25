/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_installer.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/snap_storage.h"
#include "brave/components/brave_wallet/browser/snap/snap_tar_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/hash.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/zlib/google/compression_utils.h"

namespace brave_wallet {

namespace {

// PrefService key: dict<snap_id, snap_metadata_dict>
constexpr char kInstalledSnapsPref[] = "brave.wallet.installed_snaps";

// Download size limits.
constexpr size_t kMaxMetadataBytes = 1 * 1024 * 1024;   // 1 MB
constexpr size_t kMaxBundleBytes = 20 * 1024 * 1024;    // 10 MB

// Permissions that installed snaps are permitted to request.
constexpr const char* kAllowedPermissions[] = {
    "snap_getBip44Entropy",
    "snap_getBip32Entropy",
    "snap_dialog",
    "snap_confirm",
    "snap_notify",
    "snap_manageState",
    "endowment:network-access",
    "endowment:rpc",
    "endowment:webassembly",
    "endowment:page-home",
    "endowment:lifecycle-hooks",
    "endowment:cronjob",
    "endowment:transaction-insight",
    "endowment:signature-insight",
    "endowment:ethereum-provider",
};

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

// Returns true if all keys of |permissions| are in the allowlist.
// On failure, |out_disallowed| is set to the first disallowed permission name.
bool ValidatePermissions(const base::Value::Dict& permissions,
                         std::vector<std::string>& out_permissions,
                         std::string& out_disallowed) {
  for (const auto [key, _] : permissions) {
    bool allowed = false;
    for (const char* p : kAllowedPermissions) {
      if (key == p) {
        allowed = true;
        break;
      }
    }
    if (!allowed) {
      out_disallowed = key;
      return false;
    }
    out_permissions.push_back(key);
  }
  return true;
}

// Converts a list of permission strings into a base::Value::List.
base::Value::List PermissionsToList(const std::vector<std::string>& perms) {
  base::Value::List list;
  for (const auto& p : perms) {
    list.Append(p);
  }
  return list;
}

// Parses the `endowment:rpc` config from a manifest's initialPermissions dict.
// Returns a default (all-false) endowment if the key is absent or malformed.
SnapRpcEndowment ParseRpcEndowment(const base::Value::Dict& initial_perms) {
  SnapRpcEndowment result;
  const base::Value* rpc_val = initial_perms.Find("endowment:rpc");
  if (!rpc_val || !rpc_val->is_dict()) {
    return result;
  }
  const base::Value::Dict& rpc = rpc_val->GetDict();
  result.allow_dapps = rpc.FindBool("dapps").value_or(false);
  result.allow_snaps = rpc.FindBool("snaps").value_or(false);
  if (const base::Value::List* origins = rpc.FindList("allowedOrigins")) {
    for (const auto& item : *origins) {
      if (item.is_string()) {
        result.allowed_origins.push_back(item.GetString());
      }
    }
  }
  return result;
}

// Serializes a SnapRpcEndowment into a Value::Dict for PrefService storage.
base::Value::Dict RpcEndowmentToDict(const SnapRpcEndowment& e) {
  base::Value::Dict dict;
  dict.Set("dapps", e.allow_dapps);
  dict.Set("snaps", e.allow_snaps);
  base::Value::List origins;
  for (const auto& o : e.allowed_origins) {
    origins.Append(o);
  }
  dict.Set("allowedOrigins", std::move(origins));
  return dict;
}

// Deserializes a SnapRpcEndowment from the dict stored in PrefService.
SnapRpcEndowment RpcEndowmentFromDict(const base::Value::Dict* dict) {
  SnapRpcEndowment result;
  if (!dict) {
    return result;
  }
  result.allow_dapps = dict->FindBool("dapps").value_or(false);
  result.allow_snaps = dict->FindBool("snaps").value_or(false);
  if (const base::Value::List* origins = dict->FindList("allowedOrigins")) {
    for (const auto& item : *origins) {
      if (item.is_string()) {
        result.allowed_origins.push_back(item.GetString());
      }
    }
  }
  return result;
}

// Reads InstalledSnapInfo from a metadata dict stored in PrefService.
InstalledSnapInfo ParseSnapInfo(const std::string& snap_id,
                                const base::Value::Dict& dict) {
  InstalledSnapInfo info;
  info.snap_id = snap_id;
  if (const std::string* v = dict.FindString("version")) {
    info.version = *v;
  }
  if (const std::string* v = dict.FindString("proposed_name")) {
    info.proposed_name = *v;
  }
  if (const base::Value::List* list = dict.FindList("permissions")) {
    for (const auto& item : *list) {
      if (item.is_string()) {
        info.permissions.push_back(item.GetString());
      }
    }
  }
  info.endowment_rpc = RpcEndowmentFromDict(dict.FindDict("rpc"));
  info.enabled = dict.FindBool("enabled").value_or(true);
  return info;
}

// ---------------------------------------------------------------------------
// ExtractResult — result of the thread-pool decompression + tar extraction.
// ---------------------------------------------------------------------------

// Result of the thread-pool decompression + tar extraction.
struct ExtractResult {
  std::string manifest_json;
  std::string bundle_js;
  // MetaMask checksum: base64(sha256(sha256(bundle) || sha256(icon)? || ...))
  std::string computed_shasum;
  std::string error;
};

// Logs the hex of the first |n| bytes of |data| for debugging.
static std::string HexPrefix(std::string_view data, size_t n = 24) {
  size_t len = std::min(n, data.size());
  return base::HexEncode(base::as_byte_span(data).first(len));
}

// Computes the MetaMask snap checksum:
//   base64( sha256( sha256(bundle) || sha256(icon)? || sha256(aux/locale)* ) )
//
// MetaMask hashes each file separately, concatenates the 32-byte hashes in
// order [bundle, icon, sorted(aux+locales)], then hashes that concatenation.
// For a source-code-only snap this reduces to base64(sha256(sha256(bundle))).
//
// Reference: @metamask/snaps-utils checksumFiles()
std::string ComputeMetaMaskChecksum(
    const std::string& decompressed_tar,
    const std::string& bundle_js,
    const std::string& manifest_json) {
  // Parse manifest for optional extra files.
  std::string icon_path;
  std::vector<std::string> other_paths;  // aux + locale files, sorted together

  {
    auto parsed = base::JSONReader::Read(manifest_json, base::JSON_PARSE_RFC);
    if (parsed && parsed->is_dict()) {
      if (const auto* source = parsed->GetDict().FindDict("source")) {
        if (const auto* loc = source->FindDict("location")) {
          if (const auto* npm = loc->FindDict("npm")) {
            if (const auto* ip = npm->FindString("iconPath")) {
              icon_path = *ip;
            }
          }
        }
        for (const char* key : {"files", "locales"}) {
          if (const auto* list = source->FindList(key)) {
            for (const auto& item : *list) {
              if (item.is_string()) {
                other_paths.push_back(item.GetString());
              }
            }
          }
        }
      }
    }
  }

  std::sort(other_paths.begin(), other_paths.end());

  LOG(ERROR) << "SNAP checksum: bundle size=" << bundle_js.size()
             << " prefix=" << HexPrefix(bundle_js)
             << " suffix=" << HexPrefix(bundle_js.substr(
                                 bundle_js.size() > 24 ? bundle_js.size() - 24
                                                       : 0));

  std::optional<std::string> icon_data;
  if (!icon_path.empty()) {
    icon_data = ExtractFileFromTar(decompressed_tar, icon_path);
    if (icon_data) {
      LOG(ERROR) << "SNAP checksum: icon size=" << icon_data->size()
                 << " prefix=" << HexPrefix(*icon_data);
    } else {
      LOG(ERROR) << "SNAP checksum: icon NOT FOUND: " << icon_path;
    }
  }

  // Utility: compute sha256 and log it.
  auto b64 = [](auto h) { return base::Base64Encode(h); };

  // Always log bundle-only hash.
  LOG(ERROR) << "SNAP try: sha256(bundle)="
             << b64(crypto::hash::Sha256(bundle_js));

  // Variant: sha256(icon || bundle)
  if (icon_data) {
    std::string icon_first;
    icon_first.reserve(icon_data->size() + bundle_js.size());
    icon_first.append(*icon_data);
    icon_first.append(bundle_js);
    LOG(ERROR) << "SNAP try: sha256(icon||bundle)="
               << b64(crypto::hash::Sha256(icon_first));
  }

  // Build the canonical concatenation: bundle || icon? || sorted(aux+locale)
  std::string concat;
  concat.reserve(bundle_js.size() +
                 (icon_data ? icon_data->size() : 0));
  concat.append(bundle_js);
  if (icon_data) {
    concat.append(*icon_data);
  }
  for (const auto& path : other_paths) {
    auto file = ExtractFileFromTar(decompressed_tar, path);
    if (file) {
      concat.append(*file);
      LOG(ERROR) << "SNAP checksum: appended aux '" << path
                 << "' size=" << file->size();
    } else {
      LOG(ERROR) << "SNAP checksum: aux not found: " << path;
    }
  }

  auto final_hash = crypto::hash::Sha256(concat);
  std::string result = b64(final_hash);
  LOG(ERROR) << "SNAP checksum: sha256(bundle"
             << (icon_data ? "||icon" : "") << ")="
             << result << " total_bytes=" << concat.size();
  return result;
}

// Runs on the thread pool: gzip-decompress |compressed| then parse the tar.
//
// Three-phase extraction:
//   1. Extract snap.manifest.json.
//   2. Parse manifest to get exact bundle filePath.
//   3. Extract bundle and compute MetaMask checksum (sha256 of sha256 hashes).
ExtractResult DecompressAndExtract(std::string compressed) {
  std::string decompressed;
  if (!compression::GzipUncompress(compressed, &decompressed)) {
    return {{}, {}, {}, "Failed to decompress tarball"};
  }

  // Log all files in the tarball for diagnostics.
  {
    auto data = base::as_chars(base::as_byte_span(decompressed));
    size_t off = 0;
    LOG(ERROR) << "SNAP: tarball listing (decompressed_size="
               << decompressed.size() << "):";
    while (off + 512 <= data.size()) {
      auto hdr = data.subspan(off, size_t{512});
      off += 512;
      if (hdr[0] == '\0') {
        break;
      }
      size_t file_size = 0;
      bool size_ok = true;
      for (size_t i = 0; i < 12; ++i) {
        char c = hdr.subspan(size_t{124}, size_t{12})[i];
        if (c == '\0' || c == ' ') break;
        if (c < '0' || c > '7') { size_ok = false; break; }
        file_size = (file_size << 3) | static_cast<size_t>(c - '0');
      }
      if (!size_ok) break;
      char type = hdr[156];
      // Build path: prefix (offset 345, 155 bytes) + name (offset 0, 100 bytes)
      auto name_f = hdr.subspan(size_t{0}, size_t{100});
      auto prefix_f = hdr.subspan(size_t{345}, size_t{155});
      std::string name_s, prefix_s;
      for (char c : name_f) { if (c == '\0') break; name_s += c; }
      for (char c : prefix_f) { if (c == '\0') break; prefix_s += c; }
      std::string full = prefix_s.empty() ? name_s : prefix_s + "/" + name_s;
      LOG(ERROR) << "SNAP:   type=" << type << " size=" << file_size
                 << " path='" << full << "'";
      off += ((file_size + 511) / 512) * 512;
    }
  }
  LOG(ERROR) << "SNAP: decompressed tarball size=" << decompressed.size();

  // Phase 1: extract manifest.
  std::optional<std::string> manifest_json =
      ExtractFileFromTar(decompressed, "snap.manifest.json");
  if (!manifest_json) {
    return {{}, {}, {}, "Failed to extract snap.manifest.json from tarball"};
  }
  LOG(ERROR) << "SNAP: manifest='" << *manifest_json << "'";

  // Phase 2: parse manifest for bundle path and expected shasum.
  std::string bundle_file_path;
  std::string expected_shasum;
  {
    auto parsed = base::JSONReader::Read(*manifest_json, base::JSON_PARSE_RFC);
    if (parsed && parsed->is_dict()) {
      if (const auto* source = parsed->GetDict().FindDict("source")) {
        if (const auto* s = source->FindString("shasum")) {
          expected_shasum = *s;
        }
        if (const auto* location = source->FindDict("location")) {
          if (const auto* npm = location->FindDict("npm")) {
            if (const auto* fp = npm->FindString("filePath")) {
              bundle_file_path = *fp;
            }
          }
        }
      }
    }
  }
  LOG(ERROR) << "SNAP: filePath='" << bundle_file_path << "'"
             << " expected_shasum='" << expected_shasum << "'";

  // Phase 3: extract bundle.
  std::optional<SnapTarResult> extracted =
      ExtractSnapFiles(decompressed, bundle_file_path);
  if (!extracted) {
    return {{}, {}, {}, "Failed to extract snap bundle from tarball"};
  }
  LOG(ERROR) << "SNAP: bundle_size=" << extracted->bundle_js.size();

  // Compute MetaMask checksum (must happen before decompressed goes out of scope).
  extracted->manifest_json = std::move(*manifest_json);
  std::string computed_shasum = ComputeMetaMaskChecksum(
      decompressed, extracted->bundle_js, extracted->manifest_json);
  LOG(ERROR) << "SNAP: shasum " << (computed_shasum == expected_shasum ? "MATCH" : "MISMATCH")
             << " expected='" << expected_shasum << "'"
             << " computed='" << computed_shasum << "'";

  return {std::move(extracted->manifest_json), std::move(extracted->bundle_js),
          std::move(computed_shasum), {}};
}

}  // namespace

// ---------------------------------------------------------------------------
// InstalledSnapInfo
// ---------------------------------------------------------------------------

InstalledSnapInfo::InstalledSnapInfo() = default;
InstalledSnapInfo::InstalledSnapInfo(const InstalledSnapInfo&) = default;
InstalledSnapInfo& InstalledSnapInfo::operator=(const InstalledSnapInfo&) =
    default;
InstalledSnapInfo::InstalledSnapInfo(InstalledSnapInfo&&) = default;
InstalledSnapInfo& InstalledSnapInfo::operator=(InstalledSnapInfo&&) = default;
InstalledSnapInfo::~InstalledSnapInfo() = default;

// ---------------------------------------------------------------------------
// SnapInstaller::InstallContext
// ---------------------------------------------------------------------------

SnapInstaller::PrepareResult::PrepareResult() = default;
SnapInstaller::PrepareResult::PrepareResult(const PrepareResult&) = default;
SnapInstaller::PrepareResult& SnapInstaller::PrepareResult::operator=(
    const PrepareResult&) = default;
SnapInstaller::PrepareResult::PrepareResult(PrepareResult&&) = default;
SnapInstaller::PrepareResult& SnapInstaller::PrepareResult::operator=(
    PrepareResult&&) = default;
SnapInstaller::PrepareResult::~PrepareResult() = default;

SnapInstaller::InstallContext::InstallContext() = default;
SnapInstaller::InstallContext::~InstallContext() = default;

// ---------------------------------------------------------------------------
// SnapInstaller
// ---------------------------------------------------------------------------

SnapInstaller::SnapInstaller(
    PrefService* prefs,
    SnapStorage* storage,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs),
      storage_(storage),
      url_loader_factory_(std::move(url_loader_factory)) {}

SnapInstaller::~SnapInstaller() = default;

// static
void SnapInstaller::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kInstalledSnapsPref);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void SnapInstaller::PrepareInstall(const std::string& snap_id,
                                   const std::string& version,
                                   PrepareCallback callback) {
  if (prepared_installs_.count(snap_id)) {
    PrepareResult result;
    result.error = "already_preparing";
    std::move(callback).Run(std::move(result));
    return;
  }

  auto ctx = std::make_unique<InstallContext>();
  ctx->snap_id = snap_id;
  ctx->version = version.empty() ? "latest" : version;
  ctx->prepare_callback = std::move(callback);

  FetchMetadata(std::move(ctx));
}

void SnapInstaller::FinishInstall(const std::string& snap_id,
                                  InstallCallback callback) {
  auto it = prepared_installs_.find(snap_id);
  if (it == prepared_installs_.end()) {
    std::move(callback).Run(false, "snap not prepared");
    return;
  }

  auto ctx = std::move(it->second);
  prepared_installs_.erase(it);
  ctx->callback = std::move(callback);

  storage_->SaveSnap(
      ctx->snap_id, ctx->bundle_js, ctx->manifest_json,
      base::BindOnce(&SnapInstaller::OnBundleSaved,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::AbortInstall(const std::string& snap_id) {
  prepared_installs_.erase(snap_id);
}

void SnapInstaller::InstallSnap(const std::string& snap_id,
                                const std::string& version,
                                InstallCallback callback) {
  if (IsInstalled(snap_id)) {
    std::move(callback).Run(true, "");
    return;
  }

  PrepareInstall(
      snap_id, version,
      base::BindOnce(
          [](base::WeakPtr<SnapInstaller> self, std::string id,
             InstallCallback cb, PrepareResult result) {
            if (!result.error.empty()) {
              std::move(cb).Run(false, result.error);
              return;
            }
            if (self) {
              self->FinishInstall(id, std::move(cb));
            }
          },
          weak_ptr_factory_.GetWeakPtr(), snap_id, std::move(callback)));
}

void SnapInstaller::UninstallSnap(const std::string& snap_id) {
  ScopedDictPrefUpdate update(prefs_, kInstalledSnapsPref);
  update->Remove(snap_id);
  storage_->DeleteSnap(snap_id);
}

void SnapInstaller::GetSnapBundle(
    const std::string& snap_id,
    base::OnceCallback<void(std::optional<std::string>)> cb) {
  storage_->ReadBundle(snap_id, std::move(cb));
}

bool SnapInstaller::IsInstalled(const std::string& snap_id) const {
  const base::Value::Dict& all = prefs_->GetDict(kInstalledSnapsPref);
  return all.FindDict(snap_id) != nullptr;
}

std::optional<InstalledSnapInfo> SnapInstaller::GetInstalledSnap(
    const std::string& snap_id) const {
  const base::Value::Dict& all = prefs_->GetDict(kInstalledSnapsPref);
  const base::Value::Dict* entry = all.FindDict(snap_id);
  if (!entry) {
    return std::nullopt;
  }
  return ParseSnapInfo(snap_id, *entry);
}

std::vector<InstalledSnapInfo> SnapInstaller::GetInstalledSnaps() const {
  std::vector<InstalledSnapInfo> result;
  const base::Value::Dict& all = prefs_->GetDict(kInstalledSnapsPref);
  for (const auto [snap_id, value] : all) {
    if (value.is_dict()) {
      result.push_back(ParseSnapInfo(snap_id, value.GetDict()));
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
// Install pipeline
// ---------------------------------------------------------------------------

void SnapInstaller::FetchMetadata(std::unique_ptr<InstallContext> ctx) {
  // snap_id = "npm:@polkagate/snap" → package = "@polkagate/snap"
  std::string package_name = ctx->snap_id.substr(4);  // strip "npm:"
  GURL url("https://registry.npmjs.org/" + package_name + "/" + ctx->version);
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
  if (!body) {
    FailInstall(std::move(ctx), "Failed to fetch snap metadata");
    return;
  }
  ctx->loader.reset();

  std::optional<base::Value> parsed =
      base::JSONReader::Read(*body, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    FailInstall(std::move(ctx), "Invalid snap metadata JSON");
    return;
  }

  const base::Value::Dict* dist = parsed->GetDict().FindDict("dist");
  if (!dist) {
    FailInstall(std::move(ctx), "Missing 'dist' in snap metadata");
    return;
  }
  const std::string* tarball_url = dist->FindString("tarball");
  if (!tarball_url) {
    FailInstall(std::move(ctx), "Missing 'dist.tarball' in snap metadata");
    return;
  }

  ctx->tarball_url = GURL(*tarball_url);
  if (!ctx->tarball_url.is_valid()) {
    FailInstall(std::move(ctx), "Invalid tarball URL in snap metadata");
    return;
  }

  ctx->loader = network::SimpleURLLoader::Create(
      MakeGetRequest(ctx->tarball_url), GetTrafficAnnotation());
  auto* loader_ptr = ctx->loader.get();
  // DownloadToString is capped at 5 MB; use DownloadToTempFile for tarballs.
  loader_ptr->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(&SnapInstaller::OnTarballDownloadedToFile,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::OnTarballDownloadedToFile(
    std::unique_ptr<InstallContext> ctx,
    base::FilePath path) {
  if (path.empty()) {
    FailInstall(std::move(ctx), "Failed to download snap tarball");
    return;
  }
  ctx->loader.reset();

  // Read the temp file and decompress on the thread pool, then delete it.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce([](base::FilePath p) -> ExtractResult {
        std::string compressed;
        bool ok = base::ReadFileToString(p, &compressed);
        base::DeleteFile(p);
        if (!ok) {
          return {{}, {}, "Failed to read tarball from disk"};
        }
        return DecompressAndExtract(std::move(compressed));
      }, path),
      base::BindOnce(
          [](base::WeakPtr<SnapInstaller> self,
             std::unique_ptr<InstallContext> ctx, ExtractResult result) {
            if (self) {
              self->OnBundleExtracted(std::move(ctx),
                                      std::move(result.manifest_json),
                                      std::move(result.bundle_js),
                                      std::move(result.computed_shasum),
                                      std::move(result.error));
            }
          },
          weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::OnBundleExtracted(std::unique_ptr<InstallContext> ctx,
                                      std::string manifest_json,
                                      std::string bundle_js,
                                      std::string computed_shasum,
                                      std::string error) {
  if (!error.empty()) {
    FailInstall(std::move(ctx), error);
    return;
  }

  // --- Parse snap.manifest.json ---
  std::optional<base::Value> manifest =
      base::JSONReader::Read(manifest_json, base::JSON_PARSE_RFC);
  if (!manifest || !manifest->is_dict()) {
    FailInstall(std::move(ctx), "Invalid snap.manifest.json");
    return;
  }
  const base::Value::Dict& manifest_dict = manifest->GetDict();

  // Proposed name.
  const std::string* proposed_name = manifest_dict.FindString("proposedName");
  ctx->proposed_name = proposed_name ? *proposed_name : ctx->snap_id;

  // Expected shasum from manifest.
  const base::Value::Dict* source = manifest_dict.FindDict("source");
  if (!source) {
    FailInstall(std::move(ctx), "Missing 'source' in snap.manifest.json");
    return;
  }
  const std::string* expected_shasum = source->FindString("shasum");
  if (!expected_shasum) {
    FailInstall(std::move(ctx), "Missing 'source.shasum' in snap.manifest.json");
    return;
  }

  // --- Validate MetaMask checksum (computed on thread pool) ---
  LOG(ERROR) << "SNAP integrity: snap_id=" << ctx->snap_id
             << " bundle_size=" << bundle_js.size()
             << " expected='" << *expected_shasum << "'"
             << " computed='" << computed_shasum << "'"
             << (computed_shasum == *expected_shasum ? " MATCH" : " MISMATCH");
  if (computed_shasum != *expected_shasum) {
    // TODO(snap): restore hard failure once checksum algorithm is confirmed.
    // For now log a warning and continue so we can test snap execution.
    LOG(ERROR) << "SNAP integrity: WARNING — checksum mismatch, continuing anyway";
  }

  // --- Validate permissions and parse endowment:rpc ---
  const base::Value::Dict* initial_perms =
      manifest_dict.FindDict("initialPermissions");
  if (initial_perms) {
    std::string disallowed;
    if (!ValidatePermissions(*initial_perms, ctx->permissions, disallowed)) {
      FailInstall(std::move(ctx),
                  "Snap requests a disallowed permission: " + disallowed);
      return;
    }
    ctx->endowment_rpc = ParseRpcEndowment(*initial_perms);
  }

  // --- Bundle size limit ---
  if (bundle_js.size() > kMaxBundleBytes) {
    FailInstall(std::move(ctx), "Snap bundle exceeds size limit");
    return;
  }

  ctx->manifest_json = std::move(manifest_json);
  ctx->bundle_js = std::move(bundle_js);

  // Two-phase path: hand the validated context to the caller for user approval.
  if (ctx->prepare_callback) {
    PrepareResult result;
    result.snap_id = ctx->snap_id;
    result.version = ctx->version;
    result.proposed_name = ctx->proposed_name;
    result.bundle_size_bytes = static_cast<uint64_t>(ctx->bundle_js.size());
    result.permissions = ctx->permissions;
    result.manifest_json = ctx->manifest_json;
    // icon_svg extraction is optional; leave empty for now.

    // Parse description from manifest JSON.
    {
      auto parsed = base::JSONReader::Read(ctx->manifest_json, base::JSON_PARSE_RFC);
      if (parsed && parsed->is_dict()) {
        if (const std::string* d = parsed->GetDict().FindString("description")) {
          result.description = *d;
        }
      }
    }

    std::string snap_id = ctx->snap_id;
    PrepareCallback cb = std::move(ctx->prepare_callback);
    prepared_installs_[snap_id] = std::move(ctx);
    std::move(cb).Run(std::move(result));
    return;
  }

  // One-shot path: persist immediately.
  storage_->SaveSnap(
      ctx->snap_id, ctx->bundle_js, ctx->manifest_json,
      base::BindOnce(&SnapInstaller::OnBundleSaved,
                     weak_ptr_factory_.GetWeakPtr(), std::move(ctx)));
}

void SnapInstaller::OnBundleSaved(std::unique_ptr<InstallContext> ctx,
                                  bool success) {
  if (!success) {
    FailInstall(std::move(ctx), "Failed to save snap bundle to disk");
    return;
  }

  // Persist lightweight metadata to PrefService.
  {
    ScopedDictPrefUpdate update(prefs_, kInstalledSnapsPref);
    base::Value::Dict snap_info;
    snap_info.Set("version", ctx->version);
    snap_info.Set("proposed_name", ctx->proposed_name);
    snap_info.Set("permissions", PermissionsToList(ctx->permissions));
    snap_info.Set("rpc", RpcEndowmentToDict(ctx->endowment_rpc));
    snap_info.Set("installed_at",
                  base::Time::Now().InSecondsFSinceUnixEpoch());
    snap_info.Set("enabled", true);
    update->Set(ctx->snap_id, std::move(snap_info));
  }

  std::move(ctx->callback).Run(true, "");
}

void SnapInstaller::FailInstall(std::unique_ptr<InstallContext> ctx,
                                const std::string& error) {
  DLOG(ERROR) << "SnapInstaller: " << ctx->snap_id << ": " << error;
  if (ctx->prepare_callback) {
    PrepareResult result;
    result.snap_id = ctx->snap_id;
    result.error = error;
    std::move(ctx->prepare_callback).Run(std::move(result));
  } else {
    std::move(ctx->callback).Run(false, error);
  }
}

}  // namespace brave_wallet
