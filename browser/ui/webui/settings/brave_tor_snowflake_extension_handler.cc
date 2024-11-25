// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_tor_snowflake_extension_handler.h"

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "brave/components/tor/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_allowlist.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"

namespace {
// https://chrome.google.com/webstore/detail/snowflake/mafpmfcccpbjnhfhjnllmmalhifmlcie
constexpr char kSnowflakeExtensionId[] = "mafpmfcccpbjnhfhjnllmmalhifmlcie";
}  // namespace

class SnowflakeWebstoreInstaller final
    : public extensions::WebstoreInstallWithPrompt {
 public:
  using WebstoreInstallWithPrompt::WebstoreInstallWithPrompt;

 private:
  ~SnowflakeWebstoreInstaller() final = default;

  std::unique_ptr<ExtensionInstallPrompt::Prompt> CreateInstallPrompt()
      const final {
    return nullptr;
  }
  bool ShouldShowPostInstallUI() const final { return true; }
};

BraveTorSnowflakeExtensionHandler::BraveTorSnowflakeExtensionHandler() =
    default;
BraveTorSnowflakeExtensionHandler::~BraveTorSnowflakeExtensionHandler() =
    default;

void BraveTorSnowflakeExtensionHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "brave_tor.isSnowflakeExtensionAllowed",
      base::BindRepeating(
          &BraveTorSnowflakeExtensionHandler::IsSnowflakeExtensionAllowed,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_tor.isSnowflakeExtensionEnabled",
      base::BindRepeating(
          &BraveTorSnowflakeExtensionHandler::IsSnowflakeExtensionEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_tor.enableSnowflakeExtension",
      base::BindRepeating(
          &BraveTorSnowflakeExtensionHandler::EnableSnowflakeExtension,
          base::Unretained(this)));

  observation_.Observe(GetExtensionRegistry());
}

void BraveTorSnowflakeExtensionHandler::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  DCHECK(extension);
  if (extension->id() != kSnowflakeExtensionId ||
      browser_context != web_ui()->GetWebContents()->GetBrowserContext()) {
    return;
  }
  if (IsJavascriptAllowed()) {
    FireWebUIListener("tor-snowflake-extension-enabled", base::Value(true));
  }
}

void BraveTorSnowflakeExtensionHandler::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  DCHECK(extension);
  if (extension->id() != kSnowflakeExtensionId ||
      browser_context != web_ui()->GetWebContents()->GetBrowserContext()) {
    return;
  }
  if (IsJavascriptAllowed()) {
    FireWebUIListener("tor-snowflake-extension-enabled", base::Value(false));
  }
}

void BraveTorSnowflakeExtensionHandler::OnExtensionInstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    bool is_update) {
  OnExtensionLoaded(browser_context, extension);
}

void BraveTorSnowflakeExtensionHandler::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UninstallReason reason) {
  OnExtensionUnloaded(browser_context, extension,
                      extensions::UnloadedExtensionReason::UNINSTALL);
}

extensions::ExtensionRegistry*
BraveTorSnowflakeExtensionHandler::GetExtensionRegistry() {
  return extensions::ExtensionRegistry::Get(
      web_ui()->GetWebContents()->GetBrowserContext());
}

bool BraveTorSnowflakeExtensionHandler::IsTorAllowedByPolicy() {
  const bool is_allowed =
      !g_browser_process->local_state()->IsManagedPreference(
          tor::prefs::kTorDisabled);
  return is_allowed;
}

void BraveTorSnowflakeExtensionHandler::IsSnowflakeExtensionAllowed(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(IsTorAllowedByPolicy()));
}

void BraveTorSnowflakeExtensionHandler::IsSnowflakeExtensionEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);

  const bool is_allowed = IsTorAllowedByPolicy();
  const bool is_enabled = GetExtensionRegistry()->enabled_extensions().Contains(
      kSnowflakeExtensionId);

  AllowJavascript();
  ResolveJavascriptCallback(args[0], base::Value(is_allowed && is_enabled));
}

void BraveTorSnowflakeExtensionHandler::EnableSnowflakeExtension(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 2U);

  const bool enable = args[1].GetBool();
  const bool installed = GetExtensionRegistry()->GetInstalledExtension(
                             kSnowflakeExtensionId) != nullptr;
  const bool enabled = GetExtensionRegistry()->enabled_extensions().Contains(
      kSnowflakeExtensionId);

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
      installer_ = base::MakeRefCounted<SnowflakeWebstoreInstaller>(
          kSnowflakeExtensionId, profile, /*parent_window=*/nullptr,
          base::BindOnce(
              &BraveTorSnowflakeExtensionHandler::OnSnowflakeExtensionInstalled,
              weak_factory_.GetWeakPtr(), args[0].Clone()));
      installer_->BeginInstall();
    } else {
      extension_service->EnableExtension(kSnowflakeExtensionId);
      ResolveJavascriptCallback(args[0], base::Value(true));
    }
  } else {
    installer_.reset();
    extension_service->UninstallExtension(
        kSnowflakeExtensionId, extensions::UNINSTALL_REASON_INTERNAL_MANAGEMENT,
        nullptr);
    ResolveJavascriptCallback(args[0], base::Value(true));
  }
}

void BraveTorSnowflakeExtensionHandler::OnSnowflakeExtensionInstalled(
    base::Value js_callback,
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  AllowJavascript();
  if (!success) {
    RejectJavascriptCallback(js_callback, base::Value(error));
  } else {
    ResolveJavascriptCallback(js_callback, base::Value(true));
  }
}
