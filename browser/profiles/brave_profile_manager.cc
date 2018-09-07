/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_manager.h"

#include "base/metrics/histogram_macros.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/tor/pref_names.h"
#include "brave/common/tor/tor_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"

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

void BraveProfileManager::InitProfileUserPrefs(Profile* profile) {
  if (profile->GetPath() == GetTorProfilePath()) {
    PrefService* pref_service = profile->GetPrefs();
    pref_service->SetInteger(prefs::kProfileAvatarIndex, 0);
    pref_service->SetBoolean(prefs::kProfileUsingDefaultName, false);
    pref_service
      ->SetString(prefs::kProfileName,
                  l10n_util::GetStringUTF8(IDS_PROFILES_TOR_PROFILE_NAME));
    pref_service->SetBoolean(tor::prefs::kProfileUsingTor, true);
  } else {
    ProfileManager::InitProfileUserPrefs(profile);
  }
}

Profile* BraveProfileManager::CreateProfileHelper(const base::FilePath& path) {
  TRACE_EVENT0("browser", "ProfileManager::CreateProfileHelper");
  SCOPED_UMA_HISTOGRAM_TIMER("Profile.CreateProfileHelperTime");
  Profile* profile = ProfileManager::CreateProfileHelper(path);
  if (path == GetTorProfilePath()) {
     LaunchTorProcess(profile);
  }
  return profile;
}

Profile*
BraveProfileManager::CreateProfileAsyncHelper(const base::FilePath& path,
                                              Delegate* delegate) {
  Profile* profile = ProfileManager::CreateProfileAsyncHelper(path, delegate);
  if (path == GetTorProfilePath()) {
     LaunchTorProcess(profile);
  }
  return profile;
}

void BraveProfileManager::LaunchTorProcess(Profile* profile) {
  tor::TorProfileService* tor_profile_service =
    TorProfileServiceFactory::GetForProfile(profile);
  if (tor_profile_service->GetTorPid() < 0) {
    base::FilePath path =
      g_brave_browser_process->tor_client_updater()->GetExecutablePath();
    std::string proxy =
      g_browser_process->local_state()->GetString(tor::prefs::kTorProxyString);
    tor::TorConfig config(path, proxy);
    tor_profile_service->LaunchTor(config);
  }
}
