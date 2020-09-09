/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/flags_ui/flags_ui_constants.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/feature_switch.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/common/tor/pref_names.h"
#endif

BraveDefaultExtensionsHandler::BraveDefaultExtensionsHandler()
  : weak_ptr_factory_(this) {
}

BraveDefaultExtensionsHandler::~BraveDefaultExtensionsHandler() {
}

void BraveDefaultExtensionsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "setWebTorrentEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetWebTorrentEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setBraveWalletEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetBraveWalletEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setHangoutsEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetHangoutsEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setIPFSCompanionEnabled",
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::SetIPFSCompanionEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setMediaRouterEnabled",
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::SetMediaRouterEnabled,
          base::Unretained(this)));
  // TODO(petemill): If anything outside this handler is responsible for causing
  // restart-neccessary actions, then this should be moved to a generic handler
  // and the flag should be moved to somewhere more static / singleton-like.
  web_ui()->RegisterMessageCallback(
      "getRestartNeeded",
      base::BindRepeating(&BraveDefaultExtensionsHandler::GetRestartNeeded,
                          base::Unretained(this)));
#if BUILDFLAG(ENABLE_TOR)
  web_ui()->RegisterMessageCallback(
      "setTorEnabled",
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::SetTorEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isTorEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::IsTorEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isTorManaged",
      base::BindRepeating(&BraveDefaultExtensionsHandler::IsTorManaged,
                          base::Unretained(this)));
#endif

  // Can't call this in ctor because it needs to access web_ui().
  InitializePrefCallbacks();
}

void BraveDefaultExtensionsHandler::InitializePrefCallbacks() {
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      kBraveEnabledMediaRouter,
      base::Bind(&BraveDefaultExtensionsHandler::OnMediaRouterEnabledChanged,
                 base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kEnableMediaRouter,
      base::Bind(&BraveDefaultExtensionsHandler::OnMediaRouterEnabledChanged,
                 base::Unretained(this)));
#if BUILDFLAG(ENABLE_TOR)
  local_state_change_registrar_.Init(g_brave_browser_process->local_state());
  local_state_change_registrar_.Add(
      tor::prefs::kTorDisabled,
      base::Bind(&BraveDefaultExtensionsHandler::OnTorEnabledChanged,
                 base::Unretained(this)));
#endif
}

void BraveDefaultExtensionsHandler::OnMediaRouterEnabledChanged() {
  OnRestartNeededChanged();
}

bool BraveDefaultExtensionsHandler::IsRestartNeeded() {
  bool media_router_current_pref =
      profile_->GetPrefs()->GetBoolean(prefs::kEnableMediaRouter);
  bool media_router_new_pref =
      profile_->GetPrefs()->GetBoolean(kBraveEnabledMediaRouter);
  return media_router_current_pref != media_router_new_pref;
}

void BraveDefaultExtensionsHandler::GetRestartNeeded(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0],
                            base::Value(IsRestartNeeded()));
}

void BraveDefaultExtensionsHandler::SetWebTorrentEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool enabled;
  args->GetBoolean(0, &enabled);

  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  extensions::ComponentLoader* loader = service->component_loader();

  if (enabled) {
    if (!loader->Exists(brave_webtorrent_extension_id)) {
      base::FilePath brave_webtorrent_path(FILE_PATH_LITERAL(""));
      brave_webtorrent_path =
          brave_webtorrent_path.Append(FILE_PATH_LITERAL("brave_webtorrent"));
      loader->Add(IDR_BRAVE_WEBTORRENT, brave_webtorrent_path);
    }
    service->EnableExtension(brave_webtorrent_extension_id);
  } else {
    service->DisableExtension(
        brave_webtorrent_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
  }
}

void BraveDefaultExtensionsHandler::SetHangoutsEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool enabled;
  args->GetBoolean(0, &enabled);

  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();

  if (enabled) {
    extensions::ComponentLoader* loader = service->component_loader();
    if (!loader->Exists(hangouts_extension_id)) {
      static_cast<extensions::BraveComponentLoader*>(loader)->
          ForceAddHangoutServicesExtension();
    }
    service->EnableExtension(hangouts_extension_id);
  } else {
    service->DisableExtension(
        hangouts_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
  }
}

bool BraveDefaultExtensionsHandler::IsExtensionInstalled(
    const std::string& extension_id) const {
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(
          static_cast<content::BrowserContext*>(profile_));
  return registry && registry->GetInstalledExtension(extension_id);
}

void BraveDefaultExtensionsHandler::OnInstallResult(
    const std::string& pref_name,
    bool success, const std::string& error,
    extensions::webstore_install::Result result) {
  if (result != extensions::webstore_install::Result::SUCCESS &&
      result != extensions::webstore_install::Result::LAUNCH_IN_PROGRESS) {
    profile_->GetPrefs()->SetBoolean(pref_name, false);
  }
}

void BraveDefaultExtensionsHandler::OnRestartNeededChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener(
        "brave-needs-restart-changed", base::Value(IsRestartNeeded()));
  }
}

void BraveDefaultExtensionsHandler::SetMediaRouterEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool enabled;
  args->GetBoolean(0, &enabled);

  std::string feature_name(switches::kLoadMediaRouterComponentExtension);
  enabled ? feature_name += "@1" : feature_name += "@2";
  flags_ui::PrefServiceFlagsStorage flags_storage(
      g_brave_browser_process->local_state());
  about_flags::SetFeatureEntryEnabled(&flags_storage, feature_name, true);
}

#if BUILDFLAG(ENABLE_TOR)
void BraveDefaultExtensionsHandler::SetTorEnabled(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  bool enabled;
  args->GetBoolean(0, &enabled);
  AllowJavascript();
  tor::TorProfileService::SetTorDisabled(!enabled);
}

void BraveDefaultExtensionsHandler::IsTorEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0],
      base::Value(!tor::TorProfileService::IsTorDisabled()));
}

void BraveDefaultExtensionsHandler::OnTorEnabledChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener(
        "tor-enabled-changed",
        base::Value(!tor::TorProfileService::IsTorDisabled()));
  }
}

void BraveDefaultExtensionsHandler::IsTorManaged(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  const bool is_managed = g_brave_browser_process->local_state()->
      FindPreference(tor::prefs::kTorDisabled)->IsManaged();

  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0], base::Value(is_managed));
}
#endif

void BraveDefaultExtensionsHandler::SetIPFSCompanionEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool enabled;
  args->GetBoolean(0, &enabled);

  extensions::ExtensionService* service =
  extensions::ExtensionSystem::Get(profile_)->extension_service();
  if (enabled) {
    if (!IsExtensionInstalled(ipfs_companion_extension_id)) {
      // Using FindLastActiveWithProfile() here will be fine. Of course, it can
      // return NULL but only return NULL when there was no activated window
      // with |profile_| so far. But, it's impossible at here because user can't
      // request ipfs install request w/o activating browser.
      scoped_refptr<extensions::WebstoreInstallWithPrompt> installer =
          new extensions::WebstoreInstallWithPrompt(
              ipfs_companion_extension_id, profile_,
              chrome::FindLastActiveWithProfile(profile_)->window()->
                  GetNativeWindow(),
              base::BindOnce(&BraveDefaultExtensionsHandler::OnInstallResult,
                             weak_ptr_factory_.GetWeakPtr(),
                             kIPFSCompanionEnabled));
      installer->BeginInstall();
    }
    service->EnableExtension(ipfs_companion_extension_id);
  } else {
    service->DisableExtension(
        ipfs_companion_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_USER_ACTION);
  }
}

void BraveDefaultExtensionsHandler::SetBraveWalletEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  bool enabled;
  args->GetBoolean(0, &enabled);

  extensions::ExtensionService* service =
  extensions::ExtensionSystem::Get(profile_)->extension_service();
  if (enabled) {
    service->EnableExtension(ethereum_remote_client_extension_id);
  } else {
    service->DisableExtension(
        ethereum_remote_client_extension_id,
        extensions::disable_reason::DisableReason::DISABLE_USER_ACTION);
  }
}
