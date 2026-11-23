/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_CYCLING_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_CYCLING_CONTROLLER_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"
#include "ui/events/event_observer.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class BrowserWindowInterface;

namespace views {
class EventMonitor;
}  // namespace views

// Ends cycling through tabs in most recently used order, which Ctrl+Tab starts,
// once Ctrl is released, the mouse is pressed, the window is deactivated or
// closed, or the tabs change.
class TabCyclingController : public TabStripModelObserver,
                             public ui::EventObserver,
                             public views::WidgetObserver {
 public:
  DECLARE_USER_DATA(TabCyclingController);

  explicit TabCyclingController(BrowserWindowInterface& browser);
  TabCyclingController(const TabCyclingController&) = delete;
  TabCyclingController& operator=(const TabCyclingController&) = delete;
  ~TabCyclingController() override;

  // Returns the instance owned by `browser`, or nullptr.
  static TabCyclingController* From(BrowserWindowInterface* browser);

  // Called when the tab strip starts cycling.
  void Start();

  bool is_cycling() const { return !!monitor_; }

 private:
  void Stop();

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // ui::EventObserver:
  void OnEvent(const ui::Event& event) override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetClosing(views::Widget* widget) override;

  const raw_ref<BrowserWindowInterface> browser_;
  std::unique_ptr<views::EventMonitor> monitor_;
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};
  ui::ScopedUnownedUserData<TabCyclingController> scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_CYCLING_CONTROLLER_H_
