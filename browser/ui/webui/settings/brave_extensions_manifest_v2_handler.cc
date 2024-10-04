// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_extensions_manifest_v2_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/ranges/algorithm.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"

namespace {
constexpr const char kNoScriptId[] = "doojmbjmlfjjnbmnoijecmcbfeoakpjm";
constexpr const char kUBlockId[] = "cjpalhdlnbpafiamejdnhcphjbkeiagm";
constexpr const char kUMatrixId[] = "ogfcmafjalglgifnmanfmnieipoejdcf";
constexpr const char kAdGuardId[] = "gfggjaccafhcbfogfkogggoepomehbjl";
}  // namespace

BASE_FEATURE(kExtensionsManifestV2,
             "ExtensionsManifestV2",
             base::FEATURE_DISABLED_BY_DEFAULT);

class ExtensionWebstoreInstaller final
    : public extensions::WebstoreInstallWithPrompt {
 public:
  ExtensionWebstoreInstaller(
      const std::string& webstore_item_id,
      Profile* profile,
      content::WebContents* web_contents,
      extensions::WebstoreInstallWithPrompt::Callback callback)
      : extensions::WebstoreInstallWithPrompt(
            webstore_item_id,
            profile,
            web_contents->GetTopLevelNativeWindow(),
            std::move(callback)),
        web_contents_(web_contents) {}

 private:
  ~ExtensionWebstoreInstaller() final = default;

  bool ShouldShowPostInstallUI() const final { return true; }

  std::unique_ptr<ExtensionInstallPrompt> CreateInstallUI() final {
    return std::make_unique<ExtensionInstallPrompt>(web_contents_);
  }

  const raw_ptr<content::WebContents> web_contents_ = nullptr;
};

struct ExtensionManifestV2 {
  std::string id;
  std::u16string name;
  std::u16string description;
  bool enabled = false;

  base::Value ToValue() const {
    base::Value::Dict v;
    v.Set("id", id);
    v.Set("name", name);
    v.Set("description", description);
    v.Set("enabled", enabled);
    return base::Value(std::move(v));
  }
};

BraveExtensionsManifestV2Handler::BraveExtensionsManifestV2Handler() {
  // NoScript
  extensions_.push_back({kNoScriptId,
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_MANAGE_EXTENSIONS_V2_NO_SCRIPT_NAME),
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_MANAGE_EXTENSIONS_V2_NO_SCRIPT_DESC),
                         false});

  // uBlock Origin
  extensions_.push_back(
      {kUBlockId,
       brave_l10n::GetLocalizedResourceUTF16String(
           IDS_SETTINGS_MANAGE_EXTENSIONS_V2_UBLOCK_ORIGIN_NAME),
       brave_l10n::GetLocalizedResourceUTF16String(
           IDS_SETTINGS_MANAGE_EXTENSIONS_V2_UBLOCK_ORIGIN_DESC),
       false});

  // uMatrix
  extensions_.push_back({kUMatrixId,
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_MANAGE_EXTENSIONS_V2_UMATRIX_NAME),
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_MANAGE_EXTENSIONS_V2_UMATRIX_DESC),
                         false});

  // AdGuard
  extensions_.push_back({kAdGuardId,
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_MANAGE_EXTENSIONS_V2_ADGUARD_NAME),
                         brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_SETTINGS_MANAGE_EXTENSIONS_V2_ADGUARD_DESC),
                         false});
}

BraveExtensionsManifestV2Handler::~BraveExtensionsManifestV2Handler() = default;

void BraveExtensionsManifestV2Handler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getExtensionsManifestV2",
      base::BindRepeating(
          &BraveExtensionsManifestV2Handler::GetExtensionsManifestV2,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "enableExtensionManifestV2",
      base::BindRepeating(
          &BraveExtensionsManifestV2Handler::EnableExtensionManifestV2,
          base::Unretained(this)));

  observation_.Observe(GetExtensionRegistry());
}

void BraveExtensionsManifestV2Handler::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  DCHECK(extension);
  NotifyExtensionManifestV2Changed(browser_context, extension->id(), true);
}

void BraveExtensionsManifestV2Handler::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  DCHECK(extension);
  NotifyExtensionManifestV2Changed(browser_context, extension->id(), false);
}

void BraveExtensionsManifestV2Handler::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_update) {
  OnExtensionLoaded(browser_context, extension);
}

void BraveExtensionsManifestV2Handler::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UninstallReason reason) {
  OnExtensionUnloaded(browser_context, extension,
                      extensions::UnloadedExtensionReason::UNINSTALL);
}

void BraveExtensionsManifestV2Handler::NotifyExtensionManifestV2Changed(
    content::BrowserContext* browser_context,
    const std::string& id,
    bool enabled) {
  if (!IsJavascriptAllowed() ||
      browser_context != web_ui()->GetWebContents()->GetBrowserContext()) {
    return;
  }
  auto fnd = base::ranges::find(extensions_, id, &ExtensionManifestV2::id);
  if (fnd == extensions_.end()) {
    return;
  }
  fnd->enabled = enabled;
  FireWebUIListener("brave-extension-manifest-v2-changed");
}

extensions::ExtensionRegistry*
BraveExtensionsManifestV2Handler::GetExtensionRegistry() {
  return extensions::ExtensionRegistry::Get(
      web_ui()->GetWebContents()->GetBrowserContext());
}

void BraveExtensionsManifestV2Handler::EnableExtensionManifestV2(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 3U);

  const std::string id = args[1].GetString();
  const bool enable = args[2].GetBool();
  const bool installed =
      GetExtensionRegistry()->GetInstalledExtension(id) != nullptr;
  const bool enabled =
      GetExtensionRegistry()->enabled_extensions().Contains(id);

  AllowJavascript();

  if (enable == enabled) {
    ResolveJavascriptCallback(args[0], base::Value(true));
    return;
  }

  auto* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  auto* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();

  if (enable) {
    if (!installed) {
      installer_ = base::MakeRefCounted<ExtensionWebstoreInstaller>(
          id, profile, web_ui()->GetWebContents(),
          base::BindOnce(
              &BraveExtensionsManifestV2Handler::OnExtensionManifestV2Installed,
              weak_factory_.GetWeakPtr(), args[0].Clone()));
      installer_->BeginInstall();
    } else {
      extension_service->EnableExtension(id);
      ResolveJavascriptCallback(args[0], base::Value(true));
    }
  } else {
    installer_.reset();
    extension_service->UninstallExtension(
        id, extensions::UNINSTALL_REASON_INTERNAL_MANAGEMENT, nullptr);
    ResolveJavascriptCallback(args[0], base::Value(true));
  }
}

void BraveExtensionsManifestV2Handler::GetExtensionsManifestV2(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();

  base::Value::List result;
  for (auto& e : extensions_) {
    e.enabled = GetExtensionRegistry()->enabled_extensions().Contains(e.id);
    result.Append(e.ToValue());
  }
  ResolveJavascriptCallback(args[0], base::Value(std::move(result)));
}

void BraveExtensionsManifestV2Handler::OnExtensionManifestV2Installed(
    base::Value js_callback,
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  AllowJavascript();
  if (!success &&
      result != extensions::webstore_install::Result::USER_CANCELLED) {
    RejectJavascriptCallback(js_callback, base::Value(error));
  } else {
    ResolveJavascriptCallback(js_callback, base::Value(true));
  }
}
