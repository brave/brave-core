/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_impl.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/profiles/tor_profile_impl.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/tor/pref_names.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile_destroyer.h"
#include "extensions/browser/extension_pref_value_map_factory.h"
#include "extensions/browser/extension_pref_value_map.h"
#include "extensions/common/extension_features.h"

BraveProfileImpl::BraveProfileImpl(
    const base::FilePath& path,
    Delegate* delegate,
    CreateMode create_mode,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
  : ProfileImpl(path, delegate, create_mode, io_task_runner) {
}

BraveProfileImpl::~BraveProfileImpl() {
  if (tor_profile_)
    ProfileDestroyer::DestroyOffTheRecordProfileNow(
        tor_profile_.get());
}

Profile* BraveProfileImpl::GetTorProfile() {
  if (!tor_profile_) {
    std::unique_ptr<Profile> p(CreateTorProfile());
    tor_profile_.swap(p);

    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_PROFILE_CREATED,
        content::Source<Profile>(tor_profile_.get()),
        content::NotificationService::NoDetails());
  }

  return tor_profile_.get();
}

void BraveProfileImpl::DestroyTorProfile() {
  tor_profile_.reset();
// TODO (jocelyn): This map is shared between OTR and Tor profile, should we
// clear incognito session preferences only when no OTR profile is active and
// do the same check in DestroyOffTheRecordProfile?
#if BUILDFLAG(ENABLE_EXTENSIONS)
  ExtensionPrefValueMapFactory::GetForBrowserContext(this)
      ->ClearAllIncognitoSessionOnlyPreferences();
#endif
}

bool BraveProfileImpl::HasTorProfile() {
  return tor_profile_.get() != nullptr;
}

Profile* BraveProfileImpl::CreateTorProfile() {
  TorProfileImpl* profile = new TorProfileImpl(this);
  profile->Init();

  tor::TorProfileService* tor_profile_service =
    TorProfileServiceFactory::GetForProfile(profile);

  // TODO (jocelyn): Wrap these things in TorProfileService::LaunchTor then
  // just call LaunchTor here.
  if (tor_profile_service->GetTorPid() < 0) {
    base::FilePath path =
      g_brave_browser_process->tor_client_updater()->GetExecutablePath();
    std::string proxy =
      g_browser_process->local_state()->GetString(tor::prefs::kTorProxyString);
    tor::TorConfig config(path, proxy);
    tor_profile_service->LaunchTor(config);
  }

  return profile;
}

bool BraveProfileImpl::IsSameProfile(Profile* profile) {
  Profile* tor_profile = tor_profile_.get();
  return ProfileImpl::IsSameProfile(profile) ||
    (tor_profile && profile == tor_profile);
}
