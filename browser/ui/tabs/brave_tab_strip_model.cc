/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <memory>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"

#include "brave/common/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/event.h"
#include "ui/events/event_handler.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

#if defined(OS_MACOSX)
#include "ui/views/widget/widget.h"
#endif

BraveTabStripModel::BraveTabStripModel(TabStripModelDelegate* delegate,
                                       Profile* profile)
    : TabStripModel(delegate, profile) {}

BraveTabStripModel::~BraveTabStripModel() {}

void BraveTabStripModel::SelectNextTab(UserGestureDetails detail) {
  bool isMRUEnabled =
      Profile::FromBrowserContext(GetActiveWebContents()->GetBrowserContext())
          ->GetPrefs()
          ->GetBoolean(kMRUCyclingEnabled);

  if (isMRUEnabled) {
    SelectTabMRU(false, detail);
  } else {
    SelectRelativeTab(true, detail);
  }
}

void BraveTabStripModel::SelectPreviousTab(UserGestureDetails detail) {
  bool isMRUEnabled =
      Profile::FromBrowserContext(GetActiveWebContents()->GetBrowserContext())
          ->GetPrefs()
          ->GetBoolean(kMRUCyclingEnabled);

  if (isMRUEnabled) {
    SelectTabMRU(true, detail);
  } else {
    SelectRelativeTab(false, detail);
  }
}

void BraveTabStripModel::SelectTabMRU(bool backward,
                                      UserGestureDetails detail) {
  if (current_mru_cycling_index == -1) {
    // Start cycling

    // Create a list of tab indexes sorted by time of last activation
    for (int i = 0; i < count(); ++i) {
      mru_cycle_list.push_back(i);
    }

    std::sort(mru_cycle_list.begin(), mru_cycle_list.end(),
              [this](int a, int b) {
                return GetWebContentsAt(a)->GetLastActiveTime() >
                       GetWebContentsAt(b)->GetLastActiveTime();
              });

    current_mru_cycling_index = 0;

    // Create an event handler eating all keyboard events while tabing
    ctrl_released_event_handler = std::make_unique<CtrlReleaseHandler>(this);

    // Add the event handler
    gfx::NativeWindow window =
        this->GetActiveWebContents()->GetTopLevelNativeWindow();
#if defined(OS_MACOSX)
    views::Widget::GetWidgetForNativeWindow(window)
        ->GetRootView()
        ->AddPreTargetHandler(ctrl_released_event_handler.get());
#else
    window->AddPreTargetHandler(ctrl_released_event_handler.get());
#endif
  }

  int tabCount = mru_cycle_list.size();

  if (backward) {
    current_mru_cycling_index =
        (current_mru_cycling_index - 1 % tabCount + tabCount) % tabCount;
  } else {
    current_mru_cycling_index =
        (current_mru_cycling_index + 1 % tabCount + tabCount) % tabCount;
  }

  ActivateTabAt(mru_cycle_list[current_mru_cycling_index], detail);
}

void BraveTabStripModel::StopMRUCycling() {
  current_mru_cycling_index = -1;
  mru_cycle_list.clear();

  if (ctrl_released_event_handler) {
    // Remove the event handler
    gfx::NativeWindow window =
        this->GetActiveWebContents()->GetTopLevelNativeWindow();
#if defined(OS_MACOSX)
    views::Widget::GetWidgetForNativeWindow(window)
        ->GetRootView()
        ->RemovePreTargetHandler(ctrl_released_event_handler.get());
#else
    window->RemovePreTargetHandler(ctrl_released_event_handler.get());
#endif

    ctrl_released_event_handler.reset();
  }
}

BraveTabStripModel::CtrlReleaseHandler::CtrlReleaseHandler(
    BraveTabStripModel* tab_strip)
    : tab_strip(tab_strip) {}

BraveTabStripModel::CtrlReleaseHandler::~CtrlReleaseHandler() {}

void BraveTabStripModel::CtrlReleaseHandler::OnKeyEvent(ui::KeyEvent* event) {
  if (event->key_code() == ui::VKEY_CONTROL &&
      event->type() == ui::ET_KEY_RELEASED) {
    // Ctrl key was released, stop the MRU cycling
    tab_strip->StopMRUCycling();
  } else if ((event->key_code() == ui::VKEY_TAB &&
              event->type() == ui::ET_KEY_PRESSED) ||
             (event->key_code() == ui::VKEY_PRIOR &&
              event->type() == ui::ET_KEY_PRESSED) ||
             (event->key_code() == ui::VKEY_NEXT &&
              event->type() == ui::ET_KEY_PRESSED)) {
    // Block all keys while cycling except tab,pg previous, pg next keys
  } else {
    event->StopPropagation();
  }
}
