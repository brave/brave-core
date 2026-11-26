/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_SERVICE_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_SERVICE_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/sponsored_content/sponsored_content_service_observer.h"

namespace base {
class DictValue;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace variations {
class VariationsService;
}  // namespace variations

namespace sponsored_content {

// Delivers sponsored content via Component Updater; the component stays
// registered as long as any observer has been added.
class SponsoredContentService {
 public:
  SponsoredContentService(
      variations::VariationsService* variations_service,
      component_updater::ComponentUpdateService* component_update_service);

  SponsoredContentService(const SponsoredContentService&) = delete;
  SponsoredContentService& operator=(const SponsoredContentService&) = delete;

  virtual ~SponsoredContentService();

  // Clears the pointers to objects this class does not own, ahead of the
  // destructor, before `BrowserProcessImpl::StartTearDown()` destroys them.
  void StartTearDown();

  // Adding an already-added observer replays current sponsored content
  // instead of `DCHECK`ing, so this is safe to call unconditionally.
  void AddObserver(SponsoredContentServiceObserver* observer);

  // Skips Component Updater registration, for paths that load content via
  // `LoadComponentFromOverridePath()`. Safe to call again for `observer`.
  void AddObserverForOverridePath(SponsoredContentServiceObserver* observer);

  // Unregisters the component once the last observer is removed.
  void RemoveObserver(SponsoredContentServiceObserver* observer);

  // Returns `std::nullopt` before the component is registered, or once
  // unregistered.
  const std::optional<std::string>& GetComponentId() const;

  // Re-registers for a reason other than an observer's interest, e.g. a
  // country change.
  virtual void RegisterSponsoredContentComponent();

  // A no-op if no component is currently registered.
  virtual void UnregisterSponsoredContentComponent();

  // Runs the pending update check callback only if one is due.
  void MaybeCheckForComponentUpdate();

  // Queues a check on the existing installation rather than re-registering,
  // which would race Component Updater's own cleanup.
  void ForceComponentUpdate();

  // Loads sponsored content directly from `installed_dir`, bypassing
  // Component Updater, for consumers' dev-only path-override switches.
  virtual void LoadComponentFromOverridePath(
      const base::FilePath& installed_dir);

 protected:
  virtual void LoadAndNotifyContent(const base::FilePath& installed_dir);

  void HandleSponsoredNewTabTakeoverContentCallback(
      const base::FilePath& installed_dir,
      std::optional<base::DictValue> dict);
  void HandleSponsoredSiteContentCallback(const base::FilePath& installed_dir,
                                          std::optional<base::DictValue> dict);
  void HandleSponsoredConversionConfigContentCallback(
      const base::FilePath& installed_dir,
      std::optional<base::DictValue> dict);

  void SetInstalledDirForTesting(const base::FilePath& installed_dir) {
    installed_dir_ = installed_dir;
  }

 private:
  static std::optional<base::DictValue> MaybeReadJsonFile(
      const base::FilePath& installed_dir,
      const std::string& json_filename);

  virtual std::string GetCountryCode() const;

  bool ShouldCheckForComponentUpdate() const;

  void ScheduleNextComponentUpdateCheck();
  void CheckComponentUpdate(const std::string& component_id);

  std::optional<base::Time> last_updated_at_;

  // `variations_service` can be null in test. Cleared by `StartTearDown`,
  // since `BrowserProcessImpl::StartTearDown()` destroys the real one.
  raw_ptr<variations::VariationsService> variations_service_ =
      nullptr;  // Not owned.

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_ =
      nullptr;  // Not owned.

  base::WallClockTimer update_check_timer_;
  base::RepeatingClosure update_check_callback_;

  std::optional<std::string> component_id_;
  std::optional<base::FilePath> installed_dir_;

  base::ObserverList<SponsoredContentServiceObserver> observers_;

  base::WeakPtrFactory<SponsoredContentService> weak_factory_{this};
};

}  // namespace sponsored_content

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_SERVICE_H_
