/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_clear_browsing_data.h"

#include <vector>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/scoped_multi_source_observation.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/history/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browsing_data_remover.h"

namespace content {

namespace {

using content::BraveClearBrowsingData;

class BrowsingDataRemovalWatcher
    : public content::BrowsingDataRemover::Observer {
 public:
  BrowsingDataRemovalWatcher() = default;

  void ClearBrowsingDataForLoadedProfiles(
      BraveClearBrowsingData::OnExitTestingCallback* testing_callback);

  // BrowsingDataRemover::Observer implementation.
  void OnBrowsingDataRemoverDone(uint64_t failed_data_types) override;

 private:
  bool GetClearBrowsingDataOnExitSettings(const Profile* profile,
                                          uint64_t* remove_mask,
                                          uint64_t* origin_mask);
  void Wait();

  int num_profiles_to_clear_ = 0;
  base::RunLoop run_loop_;
  // Keep track of the set of BrowsingDataRemover instances this object has
  // attached itself to as an observer. When ScopedMultiSourceObservation is
  // destroyed it removes this object as an observer from all those instances.
  base::ScopedMultiSourceObservation<content::BrowsingDataRemover,
                                     content::BrowsingDataRemover::Observer>
      observer_{this};
};

// See ClearBrowsingDataHandler::HandleClearBrowsingData which constructs the
// remove_mask and the origin_mask for the same functionality not on exit.
bool BrowsingDataRemovalWatcher::GetClearBrowsingDataOnExitSettings(
    const Profile* profile,
    uint64_t* remove_mask,
    uint64_t* origin_mask) {
  DCHECK(remove_mask);
  DCHECK(origin_mask);
  const PrefService* prefs = profile->GetPrefs();
  *remove_mask = 0;
  *origin_mask = 0;

  uint64_t site_data_mask = chrome_browsing_data_remover::DATA_TYPE_SITE_DATA;
  // Don't try to clear LSO data if it's not supported.
  if (prefs->GetBoolean(browsing_data::prefs::kDeleteBrowsingHistoryOnExit) &&
      prefs->GetBoolean(prefs::kAllowDeletingBrowserHistory)) {
    *remove_mask |= chrome_browsing_data_remover::DATA_TYPE_HISTORY;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteDownloadHistoryOnExit) &&
      prefs->GetBoolean(prefs::kAllowDeletingBrowserHistory)) {
    *remove_mask |= content::BrowsingDataRemover::DATA_TYPE_DOWNLOADS;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteCacheOnExit)) {
    *remove_mask |= content::BrowsingDataRemover::DATA_TYPE_CACHE;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteCookiesOnExit)) {
    *remove_mask |= site_data_mask;
    *origin_mask |= content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeletePasswordsOnExit)) {
    *remove_mask |= chrome_browsing_data_remover::DATA_TYPE_PASSWORDS;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteFormDataOnExit)) {
    *remove_mask |= chrome_browsing_data_remover::DATA_TYPE_FORM_DATA;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteHostedAppsDataOnExit)) {
    *remove_mask |= site_data_mask;
    *origin_mask |= content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;
  }

  // Note: this will also delete Brave Shields site-specific settings.
  // Corresponds to "Content settings" checkbox in the Clear Browsing Data
  // dialog.
  if (prefs->GetBoolean(browsing_data::prefs::kDeleteSiteSettingsOnExit)) {
    *remove_mask |= chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteBraveLeoHistoryOnExit)) {
    *remove_mask |= chrome_browsing_data_remover::DATA_TYPE_BRAVE_LEO_HISTORY;
  }

  return (*remove_mask != 0);
}

// This method will, for each loaded profile that is not off-the-record, gather
// the user specified types of data that need to be cleared. It will then get
// the BrowsingDataRemover for that profile and call its RemoveAndReply method.
// BrowsingDataRemover will create a number of tasks to clear the data. Because
// these tasks, on their own, will neither prevent shutdown nor stop the profile
// from being destroyed, we have to block shutdown execution from proceeding any
// further. Otherwise the tasks will be cancelled and the profiles destroyed.
// Since we can't actually block the UI thread, instead we implement the Wait
// method below, which just runs a RunLoop. When a BrowsingDataRemover finishes
// its tasks it will reply back to us by calling the OnBrowsingDataRemoverDone
// method below. When that happens we decrement the counter of profiles that
// need to be cleared. Once the counter reaches 0 we exit the RunLoop and let
// shutdown proceed.
void BrowsingDataRemovalWatcher::ClearBrowsingDataForLoadedProfiles(
    BraveClearBrowsingData::OnExitTestingCallback* testing_callback) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  DCHECK(profile_manager);

  std::vector<Profile*> profiles = profile_manager->GetLoadedProfiles();
  for (Profile* profile : profiles) {
    if (profile->IsOffTheRecord()) {
      continue;
    }
    uint64_t remove_mask;
    uint64_t origin_mask;
    if (!GetClearBrowsingDataOnExitSettings(profile, &remove_mask,
                                            &origin_mask)) {
      continue;
    }
    ++num_profiles_to_clear_;
    content::BrowsingDataRemover* remover = profile->GetBrowsingDataRemover();
    observer_.AddObservation(remover);
    if (testing_callback) {
      testing_callback->BeforeClearOnExitRemoveData(remover, remove_mask,
                                                    origin_mask);
    }
    remover->RemoveAndReply(base::Time(), base::Time::Max(), remove_mask,
                            origin_mask, this);
  }

  Wait();
}

void BrowsingDataRemovalWatcher::Wait() {
  if (num_profiles_to_clear_ > 0) {
    run_loop_.Run();
  }
}

void BrowsingDataRemovalWatcher::OnBrowsingDataRemoverDone(
    uint64_t failed_data_types) {
  --num_profiles_to_clear_;
  if (num_profiles_to_clear_ > 0) {
    return;
  }

  run_loop_.Quit();
}

}  // namespace

BraveClearBrowsingData::OnExitTestingCallback*
    BraveClearBrowsingData::on_exit_testing_callback_ = nullptr;

// static
void BraveClearBrowsingData::ClearOnExit() {
  TRACE_EVENT0("browser", "BraveClearBrowsingData::ClearOnExit");
  // Do not clear browsing data when the OS is ending session (logout/reboot/
  // shutdown) to avoid corrupting data if the process is killed.
  if (browser_shutdown::GetShutdownType() ==
      browser_shutdown::ShutdownType::kEndSession) {
    LOG(INFO) << "Will not clear browsing data on exit due to session ending.";
    return;
  }
  BrowsingDataRemovalWatcher watcher;
  watcher.ClearBrowsingDataForLoadedProfiles(on_exit_testing_callback_);
}

// static
void BraveClearBrowsingData::SetOnExitTestingCallback(
    OnExitTestingCallback* callback) {
  on_exit_testing_callback_ = callback;
}

}  // namespace content
