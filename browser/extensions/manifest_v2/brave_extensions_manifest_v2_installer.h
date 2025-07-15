// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_INSTALLER_H_
#define BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_INSTALLER_H_

#include <array>
#include <memory>
#include <optional>
#include <string>

#include "base/containers/fixed_flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/extensions/manifest_v2/features.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"

namespace extensions {
class CrxInstaller;
}  // namespace extensions

namespace content {
class WebContents;
}

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace extensions_mv2 {

inline constexpr char kNoScriptId[] = "bgkmgpgeempochogfoddiobpbhdfgkdi";
inline constexpr char kUBlockId[] = "jcokkipkhhgiakinbnnplhkdbjbgcgpe";
inline constexpr char kUMatrixId[] = "fplfeajmkijmaeldaknocljmmoebdgmk";
inline constexpr char kAdGuardId[] = "ejoelgckfgogkoppbgkklbbjdkjdbmen";

inline constexpr char kCwsNoScriptId[] = "doojmbjmlfjjnbmnoijecmcbfeoakpjm";
inline constexpr char kCwsUBlockId[] = "cjpalhdlnbpafiamejdnhcphjbkeiagm";
inline constexpr char kCwsUMatrixId[] = "ogfcmafjalglgifnmanfmnieipoejdcf";
inline constexpr char kCwsAdGuardId[] = "bgnkhhnnamicmpeenaelnjfhikgbkllg";

inline constexpr auto kBraveHosted =
    base::MakeFixedFlatMap<std::string_view, std::string_view>(
        {{kNoScriptId, kCwsNoScriptId},
         {kUBlockId, kCwsUBlockId},
         {kUMatrixId, kCwsUMatrixId},
         {kAdGuardId, kCwsAdGuardId}});

inline constexpr auto kCwsHosted =
    base::MakeFixedFlatMap<std::string_view, std::string_view>(
        {{kCwsNoScriptId, kNoScriptId},
         {kCwsUBlockId, kUBlockId},
         {kCwsUMatrixId, kUMatrixId},
         {kCwsAdGuardId, kAdGuardId}});

// In future there can be more brave-hosted mv2 extensions than published on
// CWS.
static_assert(kBraveHosted.size() >= kCwsHosted.size());

consteval std::array<std::string_view, kBraveHosted.size()>
GetPreconfiguredManifestV2Extensions() {
  // This can be made more idiomatic once Chromium style allows
  // std::views::keys
  std::array<std::string_view, kBraveHosted.size()> result{};
  std::ranges::transform(kBraveHosted, result.begin(),
                         [](const auto& p) { return p.first; });
  return result;
}

inline constexpr auto kPreconfiguredManifestV2Extensions =
    GetPreconfiguredManifestV2Extensions();

static_assert(kPreconfiguredManifestV2Extensions.size() == kBraveHosted.size());

bool IsKnownMV2Extension(const extensions::ExtensionId& id);
bool IsKnownCwsMV2Extension(const extensions::ExtensionId& id);

std::optional<extensions::ExtensionId> GetBraveHostedExtensionId(
    const extensions::ExtensionId& cws_extension_id);

std::optional<extensions::ExtensionId> GetCwsExtensionId(
    const extensions::ExtensionId& brave_hosted_extension_id);

class ExtensionManifestV2Installer {
 public:
  ~ExtensionManifestV2Installer();

  static std::unique_ptr<ExtensionManifestV2Installer> Create(
      const extensions::ExtensionId& extension_id,
      content::WebContents* web_contents,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      extensions::WebstoreInstallWithPrompt::Callback callback);

  static std::unique_ptr<ExtensionManifestV2Installer> CreateSilent(
      const extensions::ExtensionId& extension_id,
      content::BrowserContext* browser_context,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      extensions::WebstoreInstallWithPrompt::Callback callback);

  const extensions::ExtensionId& extension_id() const { return extension_id_; }

  void BeginInstall();

 private:
  ExtensionManifestV2Installer(
      const extensions::ExtensionId& extension_id,
      content::BrowserContext* browser_context,
      content::WebContents* web_contents,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      extensions::WebstoreInstallWithPrompt::Callback callback);

  void OnUpdateManifestResponse(std::optional<std::string> body);
  void DownloadCrx(const GURL& url);
  void OnCrxDownloaded(base::FilePath path);
  void OnInstalled(const std::optional<extensions::CrxInstallError>& error);

  const extensions::ExtensionId extension_id_;
  raw_ptr<content::BrowserContext> browser_context_ = nullptr;
  base::WeakPtr<content::WebContents> web_contents_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  extensions::WebstoreInstallWithPrompt::Callback callback_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  scoped_refptr<extensions::CrxInstaller> crx_installer_;
  bool silent_ = false;
  base::WeakPtrFactory<ExtensionManifestV2Installer> weak_factory_{this};
};

}  // namespace extensions_mv2

#endif  // BRAVE_BROWSER_EXTENSIONS_MANIFEST_V2_BRAVE_EXTENSIONS_MANIFEST_V2_INSTALLER_H_
