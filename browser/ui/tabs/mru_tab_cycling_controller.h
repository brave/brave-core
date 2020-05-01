/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_MRU_TAB_CYCLING_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TABS_MRU_TAB_CYCLING_CONTROLLER_H_

#include "ui/events/event_handler.h"

class BraveTabStripModel;

class MRUTabCyclingController {
 public:
  explicit MRUTabCyclingController(BraveTabStripModel* brave_tab_strip_model);

  ~MRUTabCyclingController();
  void StartMRUCycling();

 private:
  // To capture the release of the Ctrl key when doing a MRU cycling with
  // Ctrl-tab
  class CtrlReleaseHandler : public ui::EventHandler {
   public:
    explicit CtrlReleaseHandler(BraveTabStripModel* model);
    ~CtrlReleaseHandler() override;

   private:
    void OnKeyEvent(ui::KeyEvent* event) override;
    BraveTabStripModel* model_;
    DISALLOW_COPY_AND_ASSIGN(CtrlReleaseHandler);
  };

  std::unique_ptr<CtrlReleaseHandler> ctrl_released_event_handler_;

  BraveTabStripModel* model_;

  DISALLOW_COPY_AND_ASSIGN(MRUTabCyclingController);
};

#endif  // BRAVE_BROWSER_UI_TABS_MRU_TAB_CYCLING_CONTROLLER_H_
