/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
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

  void GetRestartNeeded(const base::Value::List& args);
  void SetWebTorrentEnabled(const base::Value::List& args);
  void SetHangoutsEnabled(const base::Value::List& args);
  void SetIPFSCompanionEnabled(const base::Value::List& args);
  void SetMediaRouterEnabled(const base::Value::List& args);
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  void SetBraveWalletEnabled(const base::Value::List& args);
#endif
  void SetWidevineEnabled(const base::Value::List& args);
  void IsWidevineEnabled(const base::Value::List& args);
  void OnWidevineEnabledChanged();
  void OnWalletTypeChanged();
  void GetDecentralizedDnsResolveMethodList(const base::Value::List& args);
  void GetEnsOffchainResolveMethodList(const base::Value::List& args);

  void InitializePrefCallbacks();

  bool IsExtensionInstalled(const std::string& extension_id) const;
  void OnInstallResult(const std::string& pref_name,
                       bool success,
                       const std::string& error,
                       extensions::webstore_install::Result result);
  void ResetWallet(const base::Value::List& args);
  void ResetTransactionInfo(const base::Value::List& args);
  void OnRestartNeededChanged();
  bool IsRestartNeeded();

#if BUILDFLAG(ENABLE_IPFS)
  void SetIPFSStorageMax(const base::Value::List& args);
  void ImportIpnsKey(const base::Value::List& args);
  void LaunchIPFSService(const base::Value::List& args);
  void ExportIPNSKey(const base::Value::List& args);

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
  void CheckIpfsNodeStatus(const base::Value::List& args);
  void NotifyNodeStatus();

  std::string dialog_key_;
  ui::SelectFileDialog::Type dialog_type_ = ui::SelectFileDialog::SELECT_NONE;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  PrefChangeRegistrar local_state_change_registrar_;
  bool was_widevine_enabled_ = false;
#endif
  raw_ptr<Profile> profile_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
#if BUILDFLAG(ENABLE_IPFS)
  base::ScopedObservation<ipfs::IpfsService, ipfs::IpfsServiceObserver>
      ipfs_service_observer_{this};
#endif
  base::WeakPtrFactory<BraveDefaultExtensionsHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
