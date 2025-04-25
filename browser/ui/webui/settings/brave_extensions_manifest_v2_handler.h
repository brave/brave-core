// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EXTENSIONS_MANIFEST_V2_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EXTENSIONS_MANIFEST_V2_HANDLER_H_

#include <string>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"

BASE_DECLARE_FEATURE(kExtensionsManifestV2);

inline constexpr char kNoScriptId[] = "doojmbjmlfjjnbmnoijecmcbfeoakpjm";
inline constexpr char kUBlockId[] = "cjpalhdlnbpafiamejdnhcphjbkeiagm";
inline constexpr char kUMatrixId[] = "ogfcmafjalglgifnmanfmnieipoejdcf";
inline constexpr char kAdGuardId[] = "gfggjaccafhcbfogfkogggoepomehbjl";

inline constexpr auto kPreconfiguredManifestV2Extensions =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 kUBlockId,
                                                 kNoScriptId,
                                                 kAdGuardId,
                                                 kUMatrixId,
                                             });

class BraveExtensionsManifestV2Handler
    : public settings::SettingsPageUIHandler,
      public extensions::ExtensionRegistryObserver {
 public:
  BraveExtensionsManifestV2Handler();
  BraveExtensionsManifestV2Handler(const BraveExtensionsManifestV2Handler&) =
      delete;
  BraveExtensionsManifestV2Handler& operator=(
      const BraveExtensionsManifestV2Handler&) = delete;
  ~BraveExtensionsManifestV2Handler() override;

 private:
  // SettingsPageUIHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  // extensions::ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;
  void OnExtensionInstalled(content::BrowserContext* browser_context,
                            const extensions::Extension* extension,
                            bool is_update) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const extensions::Extension* extension,
                              extensions::UninstallReason reason) override;

  void NotifyExtensionManifestV2Changed(
      content::BrowserContext* browser_context,
      const std::string& id);

  extensions::ExtensionRegistry* GetExtensionRegistry();

  void GetExtensionsManifestV2(const base::Value::List& args);
  void EnableExtensionManifestV2(const base::Value::List& args);
  void RemoveExtensionManifestV2(const base::Value::List& args);
  void OnExtensionManifestV2Installed(
      base::Value js_callback,
      bool success,
      const std::string& error,
      extensions::webstore_install::Result result);

  base::ScopedObservation<extensions::ExtensionRegistry,
                          extensions::ExtensionRegistryObserver>
      observation_{this};
  scoped_refptr<class ExtensionWebstoreInstaller> installer_;

  std::vector<struct ExtensionManifestV2> extensions_;

  base::WeakPtrFactory<BraveExtensionsManifestV2Handler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_EXTENSIONS_MANIFEST_V2_HANDLER_H_
