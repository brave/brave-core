/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_clear_browsing_data.h"

#include <optional>
#include <vector>

#include "base/barrier_closure.h"
#include "base/check.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
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

#if !BUILDFLAG(IS_ANDROID)
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#endif

namespace {

struct ProfileOnExitSettings {
  raw_ptr<Profile> profile = nullptr;
  uint64_t remove_mask = 0;
  uint64_t origin_mask = 0;
};

// See ClearBrowsingDataHandler::HandleClearBrowsingData which constructs the
// remove_mask and the origin_mask for the same functionality not on exit.
std::optional<ProfileOnExitSettings> GetClearBrowsingDataOnExitSettings(
    Profile* profile) {
  const PrefService* prefs = profile->GetPrefs();
  uint64_t remove_mask = 0;
  uint64_t origin_mask = 0;

  uint64_t site_data_mask = chrome_browsing_data_remover::DATA_TYPE_SITE_DATA;
  // Don't try to clear LSO data if it's not supported.
  if (prefs->GetBoolean(browsing_data::prefs::kDeleteBrowsingHistoryOnExit) &&
      prefs->GetBoolean(prefs::kAllowDeletingBrowserHistory)) {
    remove_mask |= chrome_browsing_data_remover::DATA_TYPE_HISTORY;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteDownloadHistoryOnExit) &&
      prefs->GetBoolean(prefs::kAllowDeletingBrowserHistory)) {
    remove_mask |= content::BrowsingDataRemover::DATA_TYPE_DOWNLOADS;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteCacheOnExit)) {
    remove_mask |= content::BrowsingDataRemover::DATA_TYPE_CACHE;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteCookiesOnExit)) {
    remove_mask |= site_data_mask;
    origin_mask |= content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeletePasswordsOnExit)) {
    remove_mask |= chrome_browsing_data_remover::DATA_TYPE_PASSWORDS;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteFormDataOnExit)) {
    remove_mask |= chrome_browsing_data_remover::DATA_TYPE_FORM_DATA;
  }

  if (prefs->GetBoolean(browsing_data::prefs::kDeleteHostedAppsDataOnExit)) {
    remove_mask |= site_data_mask;
    origin_mask |= content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;
  }

  // Note: this will also delete Brave Shields site-specific settings.
  // Corresponds to "Content settings" checkbox in the Clear Browsing Data
  // dialog.
  if (prefs->GetBoolean(browsing_data::prefs::kDeleteSiteSettingsOnExit)) {
    remove_mask |= chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS;
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (prefs->GetBoolean(browsing_data::prefs::kDeleteBraveLeoHistoryOnExit)) {
    remove_mask |= chrome_browsing_data_remover::DATA_TYPE_BRAVE_LEO_HISTORY;
  }
#endif

  if (remove_mask == 0) {
    return std::nullopt;
  }

  return ProfileOnExitSettings{.profile = profile,
                               .remove_mask = remove_mask,
                               .origin_mask = origin_mask};
}

// Returns "clear on exit" settings for all currently loaded profiles.
std::vector<ProfileOnExitSettings> GetOnExitSettingsForLoadedProfiles() {
  std::vector<ProfileOnExitSettings> profile_on_exit_settings;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  for (Profile* profile : profile_manager->GetLoadedProfiles()) {
    if (!profile->IsOffTheRecord()) {
      if (auto settings = GetClearBrowsingDataOnExitSettings(profile)) {
        profile_on_exit_settings.push_back(std::move(*settings));
      }
    }
  }
  return profile_on_exit_settings;
}

// An observer of browsing data removal tasks that executes a callback when the
// removal task is complete and then deletes itself. Based on
// BrowsingDataRemoverObserver in `chrome_browsing_data_lifetime_manager.cc`.
class BraveBrowsingDataRemoverObserver
    : public content::BrowsingDataRemover::Observer {
 public:
  ~BraveBrowsingDataRemoverObserver() override = default;

  // Creates an instance of the observer. The observer is destroyed when the
  // removal task is completed.
  static content::BrowsingDataRemover::Observer* Create(
      content::BrowsingDataRemover* remover,
      bool keep_browser_alive,
      base::OnceClosure on_remover_done) {
    return new BraveBrowsingDataRemoverObserver(remover, keep_browser_alive,
                                                std::move(on_remover_done));
  }

  // content::BrowsingDataRemover::Observer:
  void OnBrowsingDataRemoverDone(uint64_t failed_data_types) override {
    if (on_remover_done_) {
      std::move(on_remover_done_).Run();
    }
    delete this;
  }

 private:
  BraveBrowsingDataRemoverObserver(content::BrowsingDataRemover* remover,
                                   bool keep_browser_alive,
                                   base::OnceClosure on_remover_done)
      : on_remover_done_(std::move(on_remover_done)) {
#if !BUILDFLAG(IS_ANDROID)
    if (keep_browser_alive) {
      keep_alive_ = std::make_unique<ScopedKeepAlive>(
          KeepAliveOrigin::BROWSING_DATA_LIFETIME_MANAGER,
          KeepAliveRestartOption::DISABLED);
    }
#endif
    observation_.Observe(remover);
  }

  base::ScopedObservation<content::BrowsingDataRemover,
                          content::BrowsingDataRemover::Observer>
      observation_{this};
  base::OnceClosure on_remover_done_;
#if !BUILDFLAG(IS_ANDROID)
  std::unique_ptr<ScopedKeepAlive> keep_alive_;
#endif
};

// Starts a browsing data removal task for the specified profile and clear data
// preferences, and calls `on_remover_done` when complete. When
// `keep_browser_alive` is true, a ScopedKeepAlive will be used to keep the
// browser from shutting down.
void RemoveBrowsingData(const ProfileOnExitSettings& settings,
                        bool keep_browser_alive,
                        base::OnceClosure on_remover_done) {
  CHECK(settings.profile);
  CHECK(!settings.profile->IsOffTheRecord());

  auto* remover = settings.profile->GetBrowsingDataRemover();
  auto* observer = BraveBrowsingDataRemoverObserver::Create(
      remover, keep_browser_alive, std::move(on_remover_done));
  remover->RemoveAndReply(base::Time(), base::Time::Max(), settings.remove_mask,
                          settings.origin_mask, observer);
}

}  // namespace

BraveClearBrowsingData::OnExitTestingCallback*
    BraveClearBrowsingData::on_exit_testing_callback_ = nullptr;

// static
void BraveClearBrowsingData::ClearOnShutdown() {
  TRACE_EVENT0("browser", "BraveClearBrowsingData::ClearOnShutdown");

  // Do not clear browsing data when the OS is ending a session (logout/reboot/
  // shutdown) to avoid corrupting data if the process is killed.
  if (browser_shutdown::GetShutdownType() ==
      browser_shutdown::ShutdownType::kEndSession) {
    LOG(INFO) << "Will not clear browsing data on exit due to session ending.";
    return;
  }

  auto exit_settings = GetOnExitSettingsForLoadedProfiles();
  if (exit_settings.empty()) {
    return;
  }

  // Since this method is intended to be executed during the browser shutdown
  // phase, a ScopedKeepAlive will not prevent application shutdown. Instead, a
  // run loop is created to service the message queue until all removal tasks
  // have finished.
  base::RunLoop run_loop;
  base::RepeatingClosure on_remover_done =
      base::BarrierClosure(exit_settings.size(), run_loop.QuitClosure());

  for (auto& settings : exit_settings) {
    if (on_exit_testing_callback_) {
      on_exit_testing_callback_->BeforeClearOnExitRemoveData(
          settings.profile, settings.remove_mask, settings.origin_mask);
    }
    RemoveBrowsingData(settings, /*keep_browser_alive=*/false, on_remover_done);
  }

  // Run the message loop until all remover tasks have completed.
  run_loop.Run();
}

#if !BUILDFLAG(IS_ANDROID)
void BraveClearBrowsingData::ClearOnBrowserClosed(Profile* profile) {
  TRACE_EVENT0("browser", "BraveClearBrowsingData::ClearOnBrowserClosed");
  CHECK(profile);
  if (profile->IsOffTheRecord()) {
    return;
  }
  auto settings = GetClearBrowsingDataOnExitSettings(profile);
  if (!settings) {
    return;
  }
  if (on_exit_testing_callback_) {
    on_exit_testing_callback_->BeforeClearOnExitRemoveData(
        profile, settings->remove_mask, settings->origin_mask);
  }
  RemoveBrowsingData(*settings, /*keep_browser_alive=*/true, base::DoNothing());
}
#endif  // !BUILDFLAG(IS_ANDROID)

bool BraveClearBrowsingData::IsClearOnExitEnabledForAnyType(Profile* profile) {
  if (GetClearBrowsingDataOnExitSettings(profile)) {
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
}
