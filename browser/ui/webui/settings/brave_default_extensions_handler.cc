/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_default_extensions_handler.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "brave/components/decentralized_dns/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
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
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/utils.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/components/ipfs/pref_names.h"
#endif

BraveDefaultExtensionsHandler::BraveDefaultExtensionsHandler()
    : weak_ptr_factory_(this) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  was_widevine_enabled_ = IsWidevineOptedIn();
#endif
}

BraveDefaultExtensionsHandler::~BraveDefaultExtensionsHandler() {}

void BraveDefaultExtensionsHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
#if BUILDFLAG(IPFS_ENABLED)
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(profile_);
  if (service) {
    ipfs_service_observer_.Observe(service);
  }
  web_ui()->RegisterMessageCallback(
      "notifyIpfsNodeStatus",
      base::BindRepeating(&BraveDefaultExtensionsHandler::CheckIpfsNodeStatus,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setIPFSStorageMax",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetIPFSStorageMax,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "importIpnsKey",
      base::BindRepeating(&BraveDefaultExtensionsHandler::ImportIpnsKey,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "launchIPFSService",
      base::BindRepeating(&BraveDefaultExtensionsHandler::LaunchIPFSService,
                          base::Unretained(this)));
#endif

  web_ui()->RegisterMessageCallback(
      "setWebTorrentEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetWebTorrentEnabled,
                          base::Unretained(this)));
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  web_ui()->RegisterMessageCallback(
      "setBraveWalletEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetBraveWalletEnabled,
                          base::Unretained(this)));
#endif
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
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetMediaRouterEnabled,
                          base::Unretained(this)));
  // TODO(petemill): If anything outside this handler is responsible for causing
  // restart-neccessary actions, then this should be moved to a generic handler
  // and the flag should be moved to somewhere more static / singleton-like.
  web_ui()->RegisterMessageCallback(
      "getRestartNeeded",
      base::BindRepeating(&BraveDefaultExtensionsHandler::GetRestartNeeded,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setTorEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetTorEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isTorEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::IsTorEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isTorManaged",
      base::BindRepeating(&BraveDefaultExtensionsHandler::IsTorManaged,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setWidevineEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::SetWidevineEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isWidevineEnabled",
      base::BindRepeating(&BraveDefaultExtensionsHandler::IsWidevineEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "isDecentralizedDnsEnabled",
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::IsDecentralizedDnsEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getDecentralizedDnsResolveMethodList",
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::GetDecentralizedDnsResolveMethodList,
          base::Unretained(this)));

  // Can't call this in ctor because it needs to access web_ui().
  InitializePrefCallbacks();
}

void BraveDefaultExtensionsHandler::InitializePrefCallbacks() {
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      kBraveEnabledMediaRouter,
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::OnMediaRouterEnabledChanged,
          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kEnableMediaRouter,
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::OnMediaRouterEnabledChanged,
          base::Unretained(this)));
  local_state_change_registrar_.Init(g_browser_process->local_state());
#if BUILDFLAG(ENABLE_TOR)
  local_state_change_registrar_.Add(
      tor::prefs::kTorDisabled,
      base::BindRepeating(&BraveDefaultExtensionsHandler::OnTorEnabledChanged,
                          base::Unretained(this)));
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  local_state_change_registrar_.Add(
      kWidevineOptedIn,
      base::BindRepeating(
          &BraveDefaultExtensionsHandler::OnWidevineEnabledChanged,
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

  if (media_router_current_pref != media_router_new_pref)
    return true;

#if BUILDFLAG(ENABLE_WIDEVINE)
  if (was_widevine_enabled_ != IsWidevineOptedIn())
    return true;
#endif

  return false;
}

void BraveDefaultExtensionsHandler::GetRestartNeeded(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0], base::Value(IsRestartNeeded()));
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
      static_cast<extensions::BraveComponentLoader*>(loader)
          ->ForceAddHangoutServicesExtension();
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
  extensions::ExtensionRegistry* registry = extensions::ExtensionRegistry::Get(
      static_cast<content::BrowserContext*>(profile_));
  return registry && registry->GetInstalledExtension(extension_id);
}

void BraveDefaultExtensionsHandler::OnInstallResult(
    const std::string& pref_name,
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  if (result != extensions::webstore_install::Result::SUCCESS &&
      result != extensions::webstore_install::Result::LAUNCH_IN_PROGRESS) {
    profile_->GetPrefs()->SetBoolean(pref_name, false);
  }
}

void BraveDefaultExtensionsHandler::OnRestartNeededChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("brave-needs-restart-changed",
                      base::Value(IsRestartNeeded()));
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
      g_browser_process->local_state());
  about_flags::SetFeatureEntryEnabled(&flags_storage, feature_name, true);
}

void BraveDefaultExtensionsHandler::SetTorEnabled(const base::ListValue* args) {
#if BUILDFLAG(ENABLE_TOR)
  CHECK_EQ(args->GetSize(), 1U);
  bool enabled;
  args->GetBoolean(0, &enabled);
  AllowJavascript();
  TorProfileServiceFactory::SetTorDisabled(!enabled);
#endif
}

void BraveDefaultExtensionsHandler::IsTorEnabled(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0],
#if BUILDFLAG(ENABLE_TOR)
      base::Value(!TorProfileServiceFactory::IsTorDisabled()));
#else
      base::Value(false));
#endif
}

void BraveDefaultExtensionsHandler::OnTorEnabledChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("tor-enabled-changed",
#if BUILDFLAG(ENABLE_TOR)
                      base::Value(!TorProfileServiceFactory::IsTorDisabled()));
#else
                      base::Value(false));
#endif
  }
}

void BraveDefaultExtensionsHandler::IsTorManaged(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

#if BUILDFLAG(ENABLE_TOR)
  const bool is_managed = g_browser_process->local_state()
                              ->FindPreference(tor::prefs::kTorDisabled)
                              ->IsManaged();
#else
  const bool is_managed = false;
#endif

  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0], base::Value(is_managed));
}

void BraveDefaultExtensionsHandler::SetWidevineEnabled(
    const base::ListValue* args) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  CHECK_EQ(args->GetSize(), 1U);
  bool enabled;
  args->GetBoolean(0, &enabled);
  enabled ? EnableWidevineCdmComponent() : DisableWidevineCdmComponent();
  AllowJavascript();
#endif
}

void BraveDefaultExtensionsHandler::IsWidevineEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0],
#if BUILDFLAG(ENABLE_WIDEVINE)
                            base::Value(IsWidevineOptedIn()));
#else
                            base::Value(false));
#endif
}

void BraveDefaultExtensionsHandler::OnWidevineEnabledChanged() {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("widevine-enabled-changed",
#if BUILDFLAG(ENABLE_WIDEVINE)
                      base::Value(IsWidevineOptedIn()));
#else
                      base::Value(false));
#endif
    OnRestartNeededChanged();
  }
}


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
              chrome::FindLastActiveWithProfile(profile_)
                  ->window()
                  ->GetNativeWindow(),
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

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
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
#endif

void BraveDefaultExtensionsHandler::IsDecentralizedDnsEnabled(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args->GetList()[0],
#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
      base::Value(decentralized_dns::IsDecentralizedDnsEnabled()));
#else
      base::Value(false));
#endif
}

void BraveDefaultExtensionsHandler::GetDecentralizedDnsResolveMethodList(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 2U);
  AllowJavascript();

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
  decentralized_dns::Provider provider =
      static_cast<decentralized_dns::Provider>(args->GetList()[1].GetInt());
  ResolveJavascriptCallback(args->GetList()[0],
                            decentralized_dns::GetResolveMethodList(provider));
#else
  ResolveJavascriptCallback(args->GetList()[0],
                            base::Value(base::Value::Type::LIST));
#endif
}

#if BUILDFLAG(IPFS_ENABLED)
void BraveDefaultExtensionsHandler::LaunchIPFSService(
    const base::ListValue* args) {
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(profile_);
  if (!service) {
    return;
  }
  if (!service->IsDaemonLaunched())
    service->LaunchDaemon(base::NullCallback());
}

void BraveDefaultExtensionsHandler::SetIPFSStorageMax(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  int storage_max_gb = 0;
  args->GetInteger(0, &storage_max_gb);
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  prefs->SetInteger(kIpfsStorageMax, storage_max_gb);
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(profile_);
  if (!service) {
    return;
  }
  if (service->IsDaemonLaunched()) {
    service->RestartDaemon();
  }
}

void BraveDefaultExtensionsHandler::FileSelected(const base::FilePath& path,
                                                 int index,
                                                 void* params) {
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(profile_);
  if (!service)
    return;
  service->GetIpnsKeysManager()->ImportKey(
      path, dialog_key_,
      base::BindOnce(&BraveDefaultExtensionsHandler::OnKeyImported,
                     weak_ptr_factory_.GetWeakPtr()));
  select_file_dialog_.reset();
  dialog_key_.clear();
}

void BraveDefaultExtensionsHandler::OnKeyImported(const std::string& key,
                                                  const std::string& value,
                                                  bool success) {
  FireWebUIListener("brave-ipfs-key-imported", base::Value(key),
                    base::Value(value), base::Value(success));
}

void BraveDefaultExtensionsHandler::FileSelectionCanceled(void* params) {
  select_file_dialog_.reset();
  dialog_key_.clear();
}

void BraveDefaultExtensionsHandler::ImportIpnsKey(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(profile_);
  std::string key_name;
  args->GetString(0, &key_name);
  auto* web_contents = web_ui()->GetWebContents();
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(web_contents));
  if (!select_file_dialog_) {
    VLOG(1) << "Export already in progress";
    return;
  }
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  const base::FilePath directory = profile->last_selected_directory();
  gfx::NativeWindow parent_window = web_contents->GetTopLevelNativeWindow();
  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.allowed_paths = ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
  dialog_key_ = key_name;
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_FILE, std::u16string(), directory,
      &file_types, 0, FILE_PATH_LITERAL("key"), parent_window, nullptr);
}

void BraveDefaultExtensionsHandler::CheckIpfsNodeStatus(
    const base::ListValue* args) {
  NotifyNodeStatus();
}

void BraveDefaultExtensionsHandler::NotifyNodeStatus() {
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(profile_);
  bool launched = service && service->IsDaemonLaunched();
  FireWebUIListener("brave-ipfs-node-status-changed", base::Value(launched));
}

void BraveDefaultExtensionsHandler::OnIpfsLaunched(bool result, int64_t pid) {
  if (!IsJavascriptAllowed())
    return;
  NotifyNodeStatus();
}

void BraveDefaultExtensionsHandler::OnIpfsShutdown() {
  if (!IsJavascriptAllowed())
    return;
  NotifyNodeStatus();
}
void BraveDefaultExtensionsHandler::OnIpnsKeysLoaded(bool success) {
  if (!IsJavascriptAllowed())
    return;
  FireWebUIListener("brave-ipfs-keys-loaded", base::Value(success));
}
#endif
