/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SEARCH_ENGINES_ACTIVE_WINDOW_SEARCH_PROVIDER_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_SEARCH_ENGINES_ACTIVE_WINDOW_SEARCH_PROVIDER_MANAGER_H_

#include "base/gtest_prod_util.h"
#include "base/scoped_observation.h"
#include "components/prefs/pref_member.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class Profile;

// Setting proper default search provider for profile of activated window.
// Previously, we've used separate TemplateURLService for using different
// provider for normal and private profile because we want to use different
// provider for them. However, this caused many issues due to difference from
// chromium's usage. Chromium uses one instance for both profile.
// So, we will follow chromium's configuration(use one service instance for all
// profile). Instead, we will set appropriate provider for currently active
// window(profile - normal, private or tor). With this approach, we don't need
// to use separate TemplareURLService for normal, private(tor) profile.
// This doesn't handle guest window's provider because guest window only uses
// its own private profile. Using SearchEngineProviderService for managing guest
// window's DDG toggle button config is sufficient.
class ActiveWindowSearchProviderManager : public views::WidgetObserver {
 public:
  ActiveWindowSearchProviderManager(Profile* profile, views::Widget* widget);
  ~ActiveWindowSearchProviderManager() override;

  ActiveWindowSearchProviderManager(const ActiveWindowSearchProviderManager&) =
      delete;
  ActiveWindowSearchProviderManager& operator=(
      const ActiveWindowSearchProviderManager&) = delete;

 private:
  friend class SearchEngineProviderServiceTest;

  FRIEND_TEST_ALL_PREFIXES(SearchEngineProviderServiceTest,
                           CheckTorWindowSearchProviderTest);

  Profile* profile_ = nullptr;
  BooleanPrefMember use_alternative_search_engine_provider_;
  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};

  // views::WidgetObserver overrides:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetClosing(views::Widget* widget) override;

  void ObserveWidget(views::Widget* widget);
  void ObserveSearchEngineProviderPrefs();
  void OnPreferenceChanged();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SEARCH_ENGINES_ACTIVE_WINDOW_SEARCH_PROVIDER_MANAGER_H_
