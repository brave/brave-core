/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_APPLICATION_STATE_OBSERVER_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_APPLICATION_STATE_OBSERVER_H_

#include <memory>

#include "base/memory/weak_ptr.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/application_status_listener.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser_list_observer.h"

namespace content {
class BrowserContext;
}

#endif

namespace ephemeral_storage {

class ApplicationStateObserver
#if !BUILDFLAG(IS_ANDROID)
    : public BrowserListObserver
#endif
{
 public:
  class Observer {
   public:
    virtual void OnApplicationBecameActive() = 0;
    virtual void OnApplicationBecameInactive() = 0;

   protected:
    virtual ~Observer() = default;
  };

  explicit ApplicationStateObserver(
#if !BUILDFLAG(IS_ANDROID)
      content::BrowserContext* context
#endif  // !BUILDFLAG(IS_ANDROID
  );

  ApplicationStateObserver(const ApplicationStateObserver&) = delete;
  ApplicationStateObserver& operator=(const ApplicationStateObserver&) = delete;
  ~ApplicationStateObserver()
#if !BUILDFLAG(IS_ANDROID)
      override
#endif
      ;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
#if BUILDFLAG(IS_ANDROID)
  void TriggerCurrentAppStateNotification();
#endif

 private:
#if BUILDFLAG(IS_ANDROID)
  void OnApplicationStateChange(base::android::ApplicationState new_state);

  std::unique_ptr<base::android::ApplicationStatusListener>
      app_status_listener_;
  base::android::ApplicationState current_state_{
      base::android::APPLICATION_STATE_UNKNOWN};
#endif

#if !BUILDFLAG(IS_ANDROID)
  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
#endif

  void NotifyApplicationBecameActive();
  void NotifyApplicationBecameInactive();

  std::vector<Observer*> observers_;
  bool has_notified_active_ = false;

#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<content::BrowserContext> context_ = nullptr;
#endif  // !BUILDFLAG(IS_ANDROID)

  base::WeakPtrFactory<ApplicationStateObserver> weak_ptr_factory_{this};
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_APPLICATION_STATE_OBSERVER_H_
