/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "components/prefs/pref_change_registrar.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#endif

class Profile;

class BraveDefaultExtensionsHandler : public settings::SettingsPageUIHandler
#if BUILDFLAG(ENABLE_IPFS)
    ,
                                      ipfs::IpfsServiceObserver,
                                      public ui::SelectFileDialog::Listener
#endif
{
 public:
  BraveDefaultExtensionsHandler();
  BraveDefaultExtensionsHandler(const BraveDefaultExtensionsHandler&) = delete;
  BraveDefaultExtensionsHandler& operator=(
      const BraveDefaultExtensionsHandler&) = delete;
  ~BraveDefaultExtensionsHandler() override;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void GetRestartNeeded(base::Value::ConstListView args);
  void SetWebTorrentEnabled(base::Value::ConstListView args);
  void SetHangoutsEnabled(base::Value::ConstListView args);
  void SetIPFSCompanionEnabled(base::Value::ConstListView args);
  void SetMediaRouterEnabled(base::Value::ConstListView args);
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  void SetBraveWalletEnabled(base::Value::ConstListView args);
#endif
  void SetTorEnabled(base::Value::ConstListView args);
  void IsTorEnabled(base::Value::ConstListView args);
  void OnTorEnabledChanged();
  void IsTorManaged(base::Value::ConstListView args);
  void SetWidevineEnabled(base::Value::ConstListView args);
  void IsWidevineEnabled(base::Value::ConstListView args);
  void OnWidevineEnabledChanged();
  void IsDecentralizedDnsEnabled(base::Value::ConstListView args);
  void GetDecentralizedDnsResolveMethodList(base::Value::ConstListView args);

  void InitializePrefCallbacks();

  bool IsExtensionInstalled(const std::string& extension_id) const;
  void OnInstallResult(const std::string& pref_name,
                       bool success,
                       const std::string& error,
                       extensions::webstore_install::Result result);
  void ResetWallet(base::Value::ConstListView args);
  void ResetTransactionInfo(base::Value::ConstListView args);
  void OnRestartNeededChanged();
  bool IsRestartNeeded();

#if BUILDFLAG(ENABLE_IPFS)
  void SetIPFSStorageMax(base::Value::ConstListView args);
  void ImportIpnsKey(base::Value::ConstListView args);
  void LaunchIPFSService(base::Value::ConstListView args);
  void ExportIPNSKey(base::Value::ConstListView args);

  // ui::SelectFileDialog::Listener
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;
  void FileSelectionCanceled(void* params) override;

  void OnKeyImported(const std::string& key,
                     const std::string& value,
                     bool success);
  void OnKeyExported(const std::string& key, bool success);
  // ipfs::IpfsServiceObserver
  void OnIpfsLaunched(bool result, int64_t pid) override;
  void OnIpfsShutdown() override;
  void OnIpnsKeysLoaded(bool success) override;
  void CheckIpfsNodeStatus(base::Value::ConstListView args);
  void NotifyNodeStatus();

  std::string dialog_key_;
  ui::SelectFileDialog::Type dialog_type_ = ui::SelectFileDialog::SELECT_NONE;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  bool was_widevine_enabled_ = false;
#endif
  Profile* profile_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
#if BUILDFLAG(ENABLE_TOR)
  PrefChangeRegistrar local_state_change_registrar_;
#endif
#if BUILDFLAG(ENABLE_IPFS)
  base::ScopedObservation<ipfs::IpfsService, ipfs::IpfsServiceObserver>
      ipfs_service_observer_{this};
#endif
  base::WeakPtrFactory<BraveDefaultExtensionsHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
