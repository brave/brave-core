/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_

#include "base/scoped_observation.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "components/prefs/pref_member.h"
#include "ui/views/widget/widget_observer.h"

class BraveBrowserFrame : public BrowserFrame,
                          public views::WidgetObserver {
 public:
  explicit BraveBrowserFrame(BrowserView* browser_view);
  ~BraveBrowserFrame() override;

  // BrowserFrame overrides:
  const ui::NativeTheme* GetNativeTheme() const override;

  // views::WidgetObserver overrides:
  void OnWidgetActivationChanged(Widget* widget, bool active) override;

 private:
  BrowserView* view_;
  BooleanPrefMember use_alternative_search_engine_provider_;
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      observation_{this};

  void ObserveWidget();
  void ObserveSearchEngineProviderPrefs();
  void OnPreferenceChanged();

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserFrame);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_
