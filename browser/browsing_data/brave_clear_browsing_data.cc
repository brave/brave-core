/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_clear_browsing_data.h"

#include <map>
#include <vector>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/scoped_multi_source_observation.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/history/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/storage_partition_config.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/used_container_storage_partitions.h"
#include "brave/components/containers/core/common/features.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include <algorithm>
#include <memory>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/browser_features.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/nuke_profile_directory_utils.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace content {

namespace {

using content::BraveClearBrowsingData;

class BrowsingDataRemovalWatcher
    : public content::BrowsingDataRemover::Observer {
 public:
  BrowsingDataRemovalWatcher() = default;

  // Skips the profiles at |released_profile_paths|.
  void ClearBrowsingDataForLoadedProfiles(
      BraveClearBrowsingData::OnExitTestingCallback* testing_callback,
      const base::flat_set<base::FilePath>& released_profile_paths);

  // BrowsingDataRemover::Observer implementation.
  void OnBrowsingDataRemoverDone(uint64_t failed_data_types) override;

 private:
  void Wait();

  int num_pending_removals_ = 0;
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
bool GetClearBrowsingDataOnExitSettings(const Profile* profile,
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

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (prefs->GetBoolean(browsing_data::prefs::kDeleteBraveLeoHistoryOnExit)) {
    *remove_mask |= chrome_browsing_data_remover::DATA_TYPE_BRAVE_LEO_HISTORY;
  }
#endif

  return (*remove_mask != 0);
}

// Starts removing the data that |profile|'s *OnExit preferences select.
// |observer| must already observe |profile|'s BrowsingDataRemover. Adds one to
// |*num_pending_removals| before each removal starts, so a removal that replies
// at once never sees a stale count. Returns false when the preferences select
// nothing.
bool StartClearingOnExitData(
    Profile* profile,
    content::BrowsingDataRemover::Observer* observer,
    BraveClearBrowsingData::OnExitTestingCallback* testing_callback,
    int* num_pending_removals) {
  CHECK(num_pending_removals);
  uint64_t remove_mask;
  uint64_t origin_mask;
  if (!GetClearBrowsingDataOnExitSettings(profile, &remove_mask,
                                          &origin_mask)) {
    return false;
  }
  content::BrowsingDataRemover* remover = profile->GetBrowsingDataRemover();
  if (testing_callback) {
    testing_callback->BeforeClearOnExitRemoveData(remover, remove_mask,
                                                  origin_mask);
  }
  ++*num_pending_removals;
  remover->RemoveAndReply(base::Time(), base::Time::Max(), remove_mask,
                          origin_mask, observer);

#if BUILDFLAG(ENABLE_CONTAINERS)
  // The delegate's own container removals are not awaited, so schedule them
  // here too. The remover merges identical tasks.
  const uint64_t container_remove_mask =
      content::BrowsingDataRemover::DATA_TYPE_ON_STORAGE_PARTITION &
      remove_mask;
  if (container_remove_mask &&
      base::FeatureList::IsEnabled(containers::features::kContainers)) {
    for (const content::StoragePartitionConfig& config :
         containers::GetUsedContainerStoragePartitionConfigs(profile)) {
      auto partition_filter = content::BrowsingDataFilterBuilder::Create(
          content::BrowsingDataFilterBuilder::Mode::kPreserve);
      partition_filter->SetStoragePartitionConfig(config);
      ++*num_pending_removals;
      remover->RemoveWithFilterAndReply(base::Time(), base::Time::Max(),
                                        container_remove_mask, origin_mask,
                                        std::move(partition_filter), observer);
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

  return true;
}

#if !BUILDFLAG(IS_ANDROID)
// Returns true when a keep-alive of |origin| may mean the profile writes
// data. Data clearing does not. An open window is checked directly. A private
// window keeps its regular profile loaded, but private browsing data is kept
// apart, in memory, so it writes nothing to that profile. The macOS History
// menu holds a profile only while it loads entries for the menu.
bool MayWriteData(ProfileKeepAliveOrigin origin) {
  return origin != ProfileKeepAliveOrigin::kClearingBrowsingData &&
         origin != ProfileKeepAliveOrigin::kBrowserWindow &&
         origin != ProfileKeepAliveOrigin::kOffTheRecordProfile &&
         origin != ProfileKeepAliveOrigin::kHistoryMenuBridge;
}

// Returns true when |profile|'s *OnExit preferences select data to clear.
bool HasClearOnExitData(const Profile* profile) {
  uint64_t remove_mask;
  uint64_t origin_mask;
  return GetClearBrowsingDataOnExitSettings(profile, &remove_mask,
                                            &origin_mask);
}

// Clears one profile's *OnExit data, and keeps the application from exiting
// until every removal replies. Then it reports through |on_done|.
class ProfileDataClearer : public content::BrowsingDataRemover::Observer {
 public:
  ProfileDataClearer(const base::FilePath& profile_path,
                     base::OnceClosure on_done)
      : profile_path_(profile_path),
        // Same origin Chromium uses to finish its own on-exit clearing before
        // the application exits.
        application_keep_alive_(KeepAliveOrigin::BROWSING_DATA_LIFETIME_MANAGER,
                                KeepAliveRestartOption::DISABLED),
        on_done_(std::move(on_done)) {}
  ProfileDataClearer(const ProfileDataClearer&) = delete;
  ProfileDataClearer& operator=(const ProfileDataClearer&) = delete;
  ~ProfileDataClearer() override = default;

  const base::FilePath& profile_path() const { return profile_path_; }

  // Returns false when |profile|'s preferences select nothing to clear.
  bool Start(Profile* profile,
             BraveClearBrowsingData::OnExitTestingCallback* testing_callback) {
    observation_.Observe(profile->GetBrowsingDataRemover());
    return StartClearingOnExitData(profile, this, testing_callback,
                                   &num_pending_removals_);
  }

  // BrowsingDataRemover::Observer implementation.
  void OnBrowsingDataRemoverDone(uint64_t failed_data_types) override {
    --num_pending_removals_;
    if (num_pending_removals_ > 0) {
      return;
    }
    // The owner destroys this object in |on_done_|, so leave the observer
    // notification before that happens.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, std::move(on_done_));
  }

 private:
  const base::FilePath profile_path_;
  ScopedKeepAlive application_keep_alive_;
  base::OnceClosure on_done_;
  int num_pending_removals_ = 0;
  base::ScopedObservation<content::BrowsingDataRemover,
                          content::BrowsingDataRemover::Observer>
      observation_{this};
};

// Makes sure a profile that clears data on exit is cleared before it unloads.
//
// Each such profile is held loaded from the moment it loads. When its last
// window closes, its data is cleared. The hold is released, so the profile can
// unload, only if nothing else is using the profile by then. Otherwise the
// hold stays until the application exits, where ClearOnExit() clears the
// profile, as Brave did before profiles could unload. A released profile that
// comes back into use before it unloads is held again.
class ProfileCloseDataClearingWatcher : public BrowserCollectionObserver,
                                        public ProfileManagerObserver {
 public:
  ProfileCloseDataClearingWatcher(
      ProfileManager* profile_manager,
      BraveClearBrowsingData::OnExitTestingCallback* testing_callback)
      : profile_manager_(profile_manager), testing_callback_(testing_callback) {
    browser_observation_.Observe(GlobalBrowserCollection::GetInstance());
    profile_manager_observation_.Observe(profile_manager);
    for (Profile* profile : profile_manager->GetLoadedProfiles()) {
      HoldIfClearsOnExit(profile);
    }
  }
  ProfileCloseDataClearingWatcher(const ProfileCloseDataClearingWatcher&) =
      delete;
  ProfileCloseDataClearingWatcher& operator=(
      const ProfileCloseDataClearingWatcher&) = delete;
  ~ProfileCloseDataClearingWatcher() override = default;

  void set_testing_callback(
      BraveClearBrowsingData::OnExitTestingCallback* testing_callback) {
    testing_callback_ = testing_callback;
  }

  // Stops starting clears, and drops any that are still running, so that
  // ClearOnExit() does the last clear and waits for it. Holds stay, so held
  // profiles stay loaded until ClearOnExit() reaches them. Returns the
  // profiles cleared after their last window closed and released while
  // nothing else used them; exit does not need to clear those again.
  base::flat_set<base::FilePath> StopClearingForExit() {
    exiting_ = true;
    weak_ptr_factory_.InvalidateWeakPtrs();
    clearers_.clear();
    return released_profile_paths_;
  }

  // ProfileManagerObserver:
  void OnProfileAdded(Profile* profile) override {
    // A profile that loads again was unloaded, so an earlier release no
    // longer says anything about its data.
    released_profile_paths_.erase(profile->GetPath());
    HoldIfClearsOnExit(profile);
  }

  void OnProfileMarkedForPermanentDeletion(Profile* profile) override {
    // The profile loses all of its data, so nothing must hold it. Drop its
    // clears first, while the hold still keeps the profile and its remover
    // alive for them to stop observing.
    const base::FilePath profile_path = profile->GetPath();
    base::EraseIf(clearers_, [&](const auto& entry) {
      return entry.second->profile_path() == profile_path;
    });
    holds_.erase(profile_path);
    released_profile_paths_.erase(profile_path);
    used_while_clearing_paths_.erase(profile_path);
    reclear_counts_.erase(profile_path);
    window_teardown_checks_.erase(profile_path);
  }

  void OnKeepAliveAdded(const Profile* profile,
                        ProfileKeepAliveOrigin keep_alive_origin) override {
    if (keep_alive_origin == ProfileKeepAliveOrigin::kClearingBrowsingData) {
      return;
    }
    const base::FilePath profile_path = profile->GetPath();
    // Work that starts during a clear may write data after the remover has
    // passed. Clear again when the current clear finishes.
    if (MayWriteData(keep_alive_origin) && HasRunningClearer(profile_path)) {
      used_while_clearing_paths_.insert(profile_path);
    }
    // A released profile is in use again, so it may write data that must be
    // cleared before it unloads.
    if (!released_profile_paths_.erase(profile_path) || !profile_manager_) {
      return;
    }
    if (Profile* loaded_profile =
            profile_manager_->GetProfileByPath(profile_path)) {
      HoldIfClearsOnExit(loaded_profile);
    }
  }

  void OnProfileManagerDestroying() override {
    profile_manager_observation_.Reset();
    profile_manager_ = nullptr;
  }

  // BrowserCollectionObserver:
  void OnBrowserClosed(BrowserWindowInterface* browser) override {
    // Match ClearOnExit(): clearing while the operating system ends the
    // session risks corrupting data if the process is killed.
    if (exiting_ || browser_shutdown::GetShutdownType() ==
                        browser_shutdown::ShutdownType::kEndSession) {
      return;
    }
    Profile* profile = browser->GetProfile()->GetOriginalProfile();
    // Keep-alives only apply to regular profiles, as in
    // ProfileManager::OnBrowserClosed().
    if (profile->IsOffTheRecord() || HasOpenWindow(profile)) {
      return;
    }
    const base::FilePath profile_path = profile->GetPath();
    // A profile being deleted loses all of its data anyway, and one with
    // nothing to clear may unload right away.
    if (IsProfileDirectoryMarkedForDeletion(profile_path) ||
        !HasClearOnExitData(profile)) {
      // A clear still running from an earlier close keeps the hold until it
      // finishes, so the profile cannot unload under it.
      if (!HasRunningClearer(profile_path)) {
        holds_.erase(profile_path);
      }
      return;
    }
    // The hold normally exists already. Taking it here covers settings turned
    // on after the profile loaded. Those settings are local and change only
    // from a settings page, which needs an open window of this profile, so
    // this close always follows such a change. The closing window keeps its own
    // keep-alive until it is destroyed, after this notification, so the profile
    // is still loaded.
    if (!HoldIfClearsOnExit(profile)) {
      LOG(WARNING) << "Profile unloaded before its on-exit data was cleared.";
      return;
    }
    // A clear may still be running from an earlier close. Start another one
    // anyway: data written while the window was open must be cleared too.
    reclear_counts_.erase(profile_path);
    window_teardown_checks_.erase(profile_path);
    StartClearing(profile);
  }

 private:
  // Returns true when a window shows |profile| or one of its off-the-record
  // profiles. The closing window has already left the collection.
  static bool HasOpenWindow(Profile* profile) {
    bool has_open_window = false;
    GlobalBrowserCollection::GetInstance()->ForEach(
        [&](BrowserWindowInterface* browser) {
          if (browser->GetProfile()->GetOriginalProfile() == profile) {
            has_open_window = true;
            return false;
          }
          return true;
        });
    return has_open_window;
  }

  // Holds |profile| loaded when its preferences select data to clear on exit.
  // Returns true when |profile| is held.
  bool HoldIfClearsOnExit(Profile* profile) {
    if (profile->IsOffTheRecord() || !HasClearOnExitData(profile)) {
      return false;
    }
    const base::FilePath profile_path = profile->GetPath();
    if (holds_.contains(profile_path)) {
      return true;
    }
    std::unique_ptr<ScopedProfileKeepAlive> hold =
        ScopedProfileKeepAlive::TryAcquire(
            profile, ProfileKeepAliveOrigin::kClearingBrowsingData);
    if (!hold) {
      return false;
    }
    holds_[profile_path] = std::move(hold);
    released_profile_paths_.erase(profile_path);
    return true;
  }

  // Starts clearing |profile|'s *OnExit data. The profile must be held.
  // Returns false when the preferences select nothing to clear.
  bool StartClearing(Profile* profile) {
    const base::FilePath profile_path = profile->GetPath();
    used_while_clearing_paths_.erase(profile_path);
    // Work already running may also write data after the remover passes.
    if (IsInUse(profile_path)) {
      used_while_clearing_paths_.insert(profile_path);
    }
    const int clearer_id = next_clearer_id_++;
    auto clearer = std::make_unique<ProfileDataClearer>(
        profile_path,
        base::BindOnce(&ProfileCloseDataClearingWatcher::OnClearingDone,
                       weak_ptr_factory_.GetWeakPtr(), clearer_id));
    ProfileDataClearer* started_clearer = clearer.get();
    clearers_[clearer_id] = std::move(clearer);
    if (!started_clearer->Start(profile, testing_callback_)) {
      clearers_.erase(clearer_id);
      return false;
    }
    return true;
  }

  // Returns true when something that may write data keeps the profile at
  // |profile_path| loaded.
  bool IsInUse(const base::FilePath& profile_path) const {
    // Without the profile manager nothing is known, so assume it is in use.
    if (!profile_manager_) {
      return true;
    }
    for (const auto& [origin, count] :
         profile_manager_->GetKeepAlivesByPath(profile_path)) {
      if (count > 0 && MayWriteData(origin)) {
        return true;
      }
    }
    return false;
  }

  // Returns true when a window of the profile at |profile_path| has closed but
  // still holds the profile while it tears down.
  bool HasClosingWindow(const base::FilePath& profile_path) const {
    if (!profile_manager_) {
      return true;
    }
    const std::map<ProfileKeepAliveOrigin, int> keep_alives =
        profile_manager_->GetKeepAlivesByPath(profile_path);
    const auto window =
        keep_alives.find(ProfileKeepAliveOrigin::kBrowserWindow);
    return window != keep_alives.end() && window->second > 0;
  }

  bool HasRunningClearer(const base::FilePath& profile_path) const {
    return std::ranges::any_of(clearers_, [&](const auto& entry) {
      return entry.second->profile_path() == profile_path;
    });
  }

  void OnClearingDone(int clearer_id) {
    auto clearer = clearers_.find(clearer_id);
    if (clearer == clearers_.end()) {
      return;
    }
    const base::FilePath profile_path = clearer->second->profile_path();
    // Decide what happens to the hold first. Erasing the clearer releases the
    // application keep-alive, which may let the application exit.
    DecideHoldAfterClearing(profile_path, clearer_id);
    clearers_.erase(clearer_id);
  }

  void DecideHoldAfterClearing(const base::FilePath& profile_path,
                               int finished_clearer_id) {
    if (!profile_manager_) {
      return;
    }
    const bool has_other_clearer =
        std::ranges::any_of(clearers_, [&](const auto& entry) {
          return entry.first != finished_clearer_id &&
                 entry.second->profile_path() == profile_path;
        });
    if (has_other_clearer) {
      return;
    }
    Profile* profile = profile_manager_->GetProfileByPath(profile_path);
    if (!profile) {
      return;
    }
    // Keep holding a profile that is still in use. The next window close, or
    // ClearOnExit(), clears it. Nothing reports when that use ends, so such a
    // profile stays loaded until then, as before profiles could unload.
    if (HasOpenWindow(profile) || IsInUse(profile_path)) {
      reclear_counts_.erase(profile_path);
      window_teardown_checks_.erase(profile_path);
      return;
    }
    // A closed window may still be tearing down and writing data. Its end is
    // not reported either, but it comes soon, so check again shortly and
    // clear once more after it.
    if (HasClosingWindow(profile_path)) {
      used_while_clearing_paths_.insert(profile_path);
      int& check_count = window_teardown_checks_[profile_path];
      if (check_count < kMaxWindowTeardownChecks) {
        ++check_count;
        base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
            FROM_HERE,
            base::BindOnce(
                &ProfileCloseDataClearingWatcher::DecideHoldAfterClearing,
                weak_ptr_factory_.GetWeakPtr(), profile_path,
                kNoFinishedClearer),
            kWindowTeardownCheckDelay);
      }
      return;
    }
    window_teardown_checks_.erase(profile_path);
    // Something may have written data after the remover passed. Clear again,
    // a bounded number of times, so steady activity cannot hold off exit;
    // past that, keep holding and let ClearOnExit() clear the profile.
    if (used_while_clearing_paths_.contains(profile_path)) {
      int& reclear_count = reclear_counts_[profile_path];
      if (reclear_count >= kMaxReclearsAfterClose) {
        return;
      }
      ++reclear_count;
      // When the settings were turned off meanwhile, nothing is left to
      // clear, so the profile may unload.
      if (StartClearing(profile)) {
        return;
      }
    }
    reclear_counts_.erase(profile_path);
    holds_.erase(profile_path);
    released_profile_paths_.insert(profile_path);
  }

  static constexpr int kMaxReclearsAfterClose = 2;
  static constexpr int kMaxWindowTeardownChecks = 10;
  static constexpr base::TimeDelta kWindowTeardownCheckDelay =
      base::Milliseconds(200);
  // Passed when no clear has just finished.
  static constexpr int kNoFinishedClearer = -1;

  raw_ptr<ProfileManager> profile_manager_;
  raw_ptr<BraveClearBrowsingData::OnExitTestingCallback> testing_callback_;
  base::flat_map<base::FilePath, std::unique_ptr<ScopedProfileKeepAlive>>
      holds_;
  base::flat_set<base::FilePath> released_profile_paths_;
  // Profiles that something used, in a way that may write data, while their
  // data was being cleared.
  base::flat_set<base::FilePath> used_while_clearing_paths_;
  // Clears started after a close-time clear, per profile, since that close.
  base::flat_map<base::FilePath, int> reclear_counts_;
  // Checks scheduled while a closed window tears down, per profile.
  base::flat_map<base::FilePath, int> window_teardown_checks_;
  int next_clearer_id_ = 0;
  base::flat_map<int, std::unique_ptr<ProfileDataClearer>> clearers_;
  // Set once ClearOnExit() takes over, so no new clear starts.
  bool exiting_ = false;
  base::ScopedObservation<BrowserCollection, BrowserCollectionObserver>
      browser_observation_{this};
  base::ScopedObservation<ProfileManager, ProfileManagerObserver>
      profile_manager_observation_{this};
  base::WeakPtrFactory<ProfileCloseDataClearingWatcher> weak_ptr_factory_{this};
};

// The watcher lives from browser start until shutdown, so it never outlasts
// the browser process that owns the window collection it observes.
std::unique_ptr<ProfileCloseDataClearingWatcher>&
GetProfileCloseDataClearingWatcher() {
  static base::NoDestructor<std::unique_ptr<ProfileCloseDataClearingWatcher>>
      watcher;
  return *watcher;
}
#endif  // !BUILDFLAG(IS_ANDROID)

// Stops clearing on profile close so ClearOnExit() does the last clear, and
// returns the profiles that need no clear at exit.
base::flat_set<base::FilePath> StopProfileCloseClearingForExit() {
#if !BUILDFLAG(IS_ANDROID)
  const auto& watcher = GetProfileCloseDataClearingWatcher();
  if (watcher) {
    return watcher->StopClearingForExit();
  }
#endif  // !BUILDFLAG(IS_ANDROID)
  return {};
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
    BraveClearBrowsingData::OnExitTestingCallback* testing_callback,
    const base::flat_set<base::FilePath>& released_profile_paths) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  DCHECK(profile_manager);

  std::vector<Profile*> profiles = profile_manager->GetLoadedProfiles();
  for (Profile* profile : profiles) {
    if (profile->IsOffTheRecord() ||
        released_profile_paths.contains(profile->GetPath())) {
      continue;
    }
    observer_.AddObservation(profile->GetBrowsingDataRemover());
    StartClearingOnExitData(profile, this, testing_callback,
                            &num_pending_removals_);
  }

  Wait();
}

void BrowsingDataRemovalWatcher::Wait() {
  if (num_pending_removals_ > 0) {
    run_loop_.Run();
  }
}

void BrowsingDataRemovalWatcher::OnBrowsingDataRemoverDone(
    uint64_t failed_data_types) {
  --num_pending_removals_;
  if (num_pending_removals_ > 0) {
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
  watcher.ClearBrowsingDataForLoadedProfiles(on_exit_testing_callback_,
                                             StopProfileCloseClearingForExit());
}

#if !BUILDFLAG(IS_ANDROID)
// static
void BraveClearBrowsingData::StartClearingOnProfileClose() {
  // Without profile unloading, profiles stay loaded until exit, where
  // ClearOnExit() reaches them. Turning the feature off restores that
  // behavior in full.
  if (!base::FeatureList::IsEnabled(features::kDestroyProfileOnBrowserClose)) {
    return;
  }
  GetProfileCloseDataClearingWatcher() =
      std::make_unique<ProfileCloseDataClearingWatcher>(
          g_browser_process->profile_manager(), on_exit_testing_callback_);
}

// static
void BraveClearBrowsingData::StopClearingOnProfileClose() {
  GetProfileCloseDataClearingWatcher().reset();
}
#endif  // !BUILDFLAG(IS_ANDROID)

bool BraveClearBrowsingData::IsClearOnExitEnabledForAnyType(Profile* profile) {
  uint64_t remove_mask;
  uint64_t origin_mask;
  if (GetClearBrowsingDataOnExitSettings(profile, &remove_mask, &origin_mask)) {
    return true;
  }
  const base::ListValue& clear_on_exit_list = profile->GetPrefs()->GetList(
      browsing_data::prefs::kClearBrowsingDataOnExitList);
  return !clear_on_exit_list.empty();
}

// static
void BraveClearBrowsingData::SetOnExitTestingCallback(
    OnExitTestingCallback* callback) {
  on_exit_testing_callback_ = callback;
#if !BUILDFLAG(IS_ANDROID)
  if (const auto& watcher = GetProfileCloseDataClearingWatcher()) {
    watcher->set_testing_callback(callback);
  }
#endif  // !BUILDFLAG(IS_ANDROID)
}

}  // namespace content
