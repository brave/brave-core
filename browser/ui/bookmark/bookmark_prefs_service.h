/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BOOKMARK_BOOKMARK_PREFS_SERVICE_H_
#define BRAVE_BROWSER_UI_BOOKMARK_BOOKMARK_PREFS_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/browser_thread.h"

class PrefService;
class Profile;

class BookmarkPrefsService : public KeyedService {
 public:
  explicit BookmarkPrefsService(Profile* profile);
  BookmarkPrefsService(const BookmarkPrefsService&) = delete;
  BookmarkPrefsService& operator=(const BookmarkPrefsService&) = delete;
  ~BookmarkPrefsService() override;

 private:
  void OnPreferenceChanged();

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
};

#endif  // BRAVE_BROWSER_UI_BOOKMARK_BOOKMARK_PREFS_SERVICE_H_
