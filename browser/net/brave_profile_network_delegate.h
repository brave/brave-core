/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_PROFILE_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_BRAVE_PROFILE_NETWORK_DELEGATE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/net/brave_network_delegate_base.h"

class PrefService;

class BraveProfileNetworkDelegate : public BraveNetworkDelegateBase {
 public:
  explicit BraveProfileNetworkDelegate(
      extensions::EventRouterForwarder* event_router);
  ~BraveProfileNetworkDelegate() override;

 private:
  void InitPrefChangeRegistrar();
  void InitPrefChangeRegistrarOnUI(const base::FilePath& profile_path);
  void OnPreferenceChanged(PrefService* user_prefs,
                           const std::string& pref_name);
  void UpdateGoogleAuthOnIO(bool allow_goole_auth);

  std::unique_ptr<PrefChangeRegistrar, content::BrowserThread::DeleteOnUIThread>
      user_pref_change_registrar_;

  base::WeakPtrFactory<BraveProfileNetworkDelegate> weak_ptr_factory_io_;
  base::WeakPtrFactory<BraveProfileNetworkDelegate> weak_ptr_factory_ui_;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileNetworkDelegate);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_PROFILE_NETWORK_DELEGATE_H_
