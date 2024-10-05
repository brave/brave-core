/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_MANAGER_H_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_MANAGER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "url/gurl.h"

class Browser;
class BrowserListObserver;

class TorProfileManager : public ProfileObserver {
 public:
  static TorProfileManager& GetInstance();
  static Browser* SwitchToTorProfile(Profile* original_profile,
                                     const GURL& url = GURL());
  static void CloseTorProfileWindows(Profile* tor_profile);
  Profile* GetTorProfile(Profile* original_profile);

  // Close all Tor windows for all tor profiles
  void CloseAllTorWindows();

 private:
  friend class base::NoDestructor<TorProfileManager>;
  TorProfileManager();
  ~TorProfileManager() override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

  void InitTorProfileUserPrefs(Profile* profile);

  // One regular profile can only have one tor profile
  base::flat_map<std::string, Profile*> tor_profiles_;
  std::unique_ptr<BrowserListObserver> browser_list_observer_;

  TorProfileManager(const TorProfileManager&) = delete;
  TorProfileManager& operator=(const TorProfileManager&) = delete;
};

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_MANAGER_H_
