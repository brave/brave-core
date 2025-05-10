// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EXTENSIONS_MANIFEST_V2_INSTALLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EXTENSIONS_MANIFEST_V2_INSTALLER_H_

#include <memory>
#include <string>

#include "base/containers/fixed_flat_set.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
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

// For metrics
inline constexpr auto kPreconfiguredManifestV2Extensions =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 kNoScriptId,
                                                 kAdGuardId,
                                                 kUMatrixId,
                                                 kUBlockId,
                                             });

class ExtensionManifestV2Installer {
 public:
  ExtensionManifestV2Installer(
      const std::string& extension_id,
      content::WebContents* web_contents,
      extensions::WebstoreInstallWithPrompt::Callback callback);
  ~ExtensionManifestV2Installer();

  void BeginInstall();

 private:
  void OnUpdateManifestResponse(std::optional<std::string> body);
  void DownloadCrx(const GURL& url);
  void OnCrxDownloaded(base::FilePath path);
  void OnInstalled(const std::optional<extensions::CrxInstallError>& error);

  const std::string extension_id_;
  const raw_ptr<content::WebContents> web_contents_;
  extensions::WebstoreInstallWithPrompt::Callback callback_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  scoped_refptr<extensions::CrxInstaller> crx_installer_;
  base::WeakPtrFactory<ExtensionManifestV2Installer> weak_factory_{this};
};

}  // namespace extensions_mv2

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EXTENSIONS_MANIFEST_V2_INSTALLER_H_
