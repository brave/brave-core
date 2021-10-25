/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_throbber.h"

#include "base/bind.h"
#include "base/location.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_throbber.h"

BraveWaybackMachineInfoBarThrobber::BraveWaybackMachineInfoBarThrobber() {
}

BraveWaybackMachineInfoBarThrobber::~BraveWaybackMachineInfoBarThrobber() {
  Stop();
}

void BraveWaybackMachineInfoBarThrobber::Start() {
  if (IsRunning())
    return;

  start_time_ = base::TimeTicks::Now();
  timer_.Start(
      FROM_HERE, base::Milliseconds(30),
      base::BindRepeating(&BraveWaybackMachineInfoBarThrobber::SchedulePaint,
                          base::Unretained(this)));
  SchedulePaint();  // paint right away
}

void BraveWaybackMachineInfoBarThrobber::Stop() {
  if (!IsRunning())
    return;

  timer_.Stop();
  SchedulePaint();
}

void BraveWaybackMachineInfoBarThrobber::OnPaint(gfx::Canvas* canvas) {
  if (!IsRunning())
    return;

  base::TimeDelta elapsed_time = base::TimeTicks::Now() - start_time_;
  gfx::PaintThrobberSpinning(
      canvas, GetContentsBounds(), SK_ColorWHITE, elapsed_time);
}

bool BraveWaybackMachineInfoBarThrobber::IsRunning() const {
  return timer_.IsRunning();
}
