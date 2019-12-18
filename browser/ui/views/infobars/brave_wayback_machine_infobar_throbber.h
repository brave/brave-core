/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_THROBBER_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_THROBBER_H_

#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/views/view.h"

// White throbber. Most of codes are copied from views::Throbber.
class BraveWaybackMachineInfoBarThrobber : public views::View {
 public:
  BraveWaybackMachineInfoBarThrobber();
  ~BraveWaybackMachineInfoBarThrobber() override;

  BraveWaybackMachineInfoBarThrobber(
      const BraveWaybackMachineInfoBarThrobber&) = delete;
  BraveWaybackMachineInfoBarThrobber& operator=(
      const BraveWaybackMachineInfoBarThrobber&) = delete;

  // Start and stop the throbber animation.
  void Start();
  void Stop();

 private:
  // Overridden from View:
  void OnPaint(gfx::Canvas* canvas) override;

    // Specifies whether the throbber is currently animating or not
  bool IsRunning() const;

  base::TimeTicks start_time_;  // Time when Start was called.
  base::RepeatingTimer timer_;  // Used to schedule Run calls.
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_THROBBER_H_
