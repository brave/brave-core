/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_manager.h"

#include "base/metrics/histogram_macros.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/tor/pref_names.h"
#include "brave/common/tor/tor_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

BraveProfileManager::BraveProfileManager(const base::FilePath& user_data_dir)
  : ProfileManager(user_data_dir) {}

//static
base::FilePath BraveProfileManager::GetTorProfilePath() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ProfileManager* profile_manager = g_browser_process->profile_manager();

  base::FilePath tor_path = profile_manager->user_data_dir();
  return tor_path.Append(tor::kTorProfileDir);
}

Profile* BraveProfileManager::CreateProfileHelper(const base::FilePath& path) {
  TRACE_EVENT0("browser", "ProfileManager::CreateProfileHelper");
  SCOPED_UMA_HISTOGRAM_TIMER("Profile.CreateProfileHelperTime");
  Profile* profile = ProfileManager::CreateProfileHelper(path);
  if (path == GetTorProfilePath()) {
     PrefService* pref_service = profile->GetPrefs();
     pref_service->SetBoolean(prefs::kProfileUsingDefaultName, false);
     pref_service->SetString(prefs::kProfileName, tor::kTorProfileName);
     pref_service->SetBoolean(tor::prefs::kProfileUsingTor, true);
     LaunchTorProcess(profile);
  }
  return profile;
}

Profile*
BraveProfileManager::CreateProfileAsyncHelper(const base::FilePath& path,
                                              Delegate* delegate) {
  Profile* profile = ProfileManager::CreateProfileAsyncHelper(path, delegate);
  if (path == GetTorProfilePath()) {
     PrefService* pref_service = profile->GetPrefs();
     pref_service->SetBoolean(prefs::kProfileUsingDefaultName, false);
     pref_service->SetString(prefs::kProfileName, tor::kTorProfileName);
     pref_service->SetBoolean(tor::prefs::kProfileUsingTor, true);
     LaunchTorProcess(profile);
  }
  return profile;
}

void BraveProfileManager::LaunchTorProcess(Profile* profile) {
  tor::TorProfileService* tor_profile_service =
    TorProfileServiceFactory::GetForProfile(profile);
  if (tor_profile_service->GetTorPid() < 0) {
    // TODO: read config from prefs
    base::FilePath path("/usr/local/bin/tor");
    std::string proxy("socks5://127.0.0.1:9050");
    tor::TorConfig config(path, proxy);
    tor_profile_service->LaunchTor(config);
  }
}
