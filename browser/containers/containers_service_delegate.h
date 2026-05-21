// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_CONTAINERS_CONTAINERS_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_CONTAINERS_CONTAINERS_SERVICE_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "chrome/common/buildflags.h"
#include "components/sessions/core/tab_restore_service_observer.h"

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/sessions/session_service_base_observer.h"
#include "components/sessions/core/session_id.h"
#endif

class Profile;
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
class SessionService;
#endif

namespace sessions {
class TabRestoreService;
struct SessionWindow;
}  // namespace sessions

// Collects container ids from the last saved session (when enabled), tab
// restore entries, and currently open tabs, then reports the union to
// ContainersService.
class ContainersServiceDelegate
    : public containers::ContainersService::Delegate,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
      public SessionServiceBaseObserver,
#endif
      public sessions::TabRestoreServiceObserver {
 public:
  ContainersServiceDelegate(Profile* profile,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                            SessionService* session_service,
#endif
                            sessions::TabRestoreService* tab_restore_service);
  ~ContainersServiceDelegate() override;

  ContainersServiceDelegate(const ContainersServiceDelegate&) = delete;
  ContainersServiceDelegate& operator=(const ContainersServiceDelegate&) =
      delete;

  // Starts async collection of container ids (from last session, tab restore,
  // open tabs). Invokes |callback| once with the merged set; see
  // MaybeRunOnReferencedContainerIdsLoaded().
  void GetReferencedContainerIds(
      OnReferencedContainerIdsReadyCallback callback) override;

  // Clears the container's storage partition, then deletes its on-disk
  // directory under the profile. |callback| receives whether deletion
  // succeeded.
  void DeleteContainerStorage(const std::string& id,
                              DeleteContainerStorageCallback callback) override;

 private:
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  // SessionServiceBaseObserver:
  void OnDestroying(SessionServiceBase* service) override;

  // Asks SessionService for the previous browser session and merges container
  // ids from serialized navigations into |pending_referenced_container_ids_|.
  void RequestLastSessionContainerReferences();
  void OnGotLastSession(
      std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
      SessionID active_window_id,
      bool read_error);
#endif

  // sessions::TabRestoreServiceObserver:
  void TabRestoreServiceDestroyed(
      sessions::TabRestoreService* service) override;
  void TabRestoreServiceLoaded(sessions::TabRestoreService* service) override;

  // Ensures tab restore is loading if needed, then checks whether all sources
  // are ready (with MaybeRunOnReferencedContainerIdsLoaded).
  void RequestTabRestoreContainerReferences();

  // When the last-session result (if enabled) and tab restore are ready, merges
  // tab restore entries and open tabs into |pending_referenced_container_ids_|,
  // then posts |on_referenced_container_ids_loaded_| with that set.
  void MaybeRunOnReferencedContainerIdsLoaded();

  const raw_ref<Profile> profile_;
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
  raw_ptr<SessionService> session_service_ = nullptr;
  base::ScopedObservation<SessionService, SessionServiceBaseObserver>
      session_service_observation_{this};
  // Set after RequestLastSessionContainerReferences completes (including
  // no-op).
  bool got_last_session_ = false;
#endif
  raw_ptr<sessions::TabRestoreService> tab_restore_service_ = nullptr;
  base::ScopedObservation<sessions::TabRestoreService,
                          sessions::TabRestoreServiceObserver>
      tab_restore_service_observation_{this};

  // Callback passed to the most recent GetReferencedContainerIds; cleared when
  // invoked.
  OnReferencedContainerIdsReadyCallback on_referenced_container_ids_loaded_;
  // Accumulates ids from last session and tab restore before the final merge
  // with open tabs.
  base::flat_set<std::string> pending_referenced_container_ids_;

  base::WeakPtrFactory<ContainersServiceDelegate> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_CONTAINERS_CONTAINERS_SERVICE_DELEGATE_H_
