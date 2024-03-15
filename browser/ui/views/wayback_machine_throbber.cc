/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/wayback_machine_throbber.h"

#include "base/functional/bind.h"
#include "base/location.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_throbber.h"

WaybackMachineThrobber::WaybackMachineThrobber() =
    default;

WaybackMachineThrobber::~WaybackMachineThrobber() {
  Stop();
}

void WaybackMachineThrobber::Start() {
  if (IsRunning())
    return;

  start_time_ = base::TimeTicks::Now();
  timer_.Start(
      FROM_HERE, base::Milliseconds(30),
      base::BindRepeating(&WaybackMachineThrobber::SchedulePaint,
                          base::Unretained(this)));
  SchedulePaint();  // paint right away
}

void WaybackMachineThrobber::Stop() {
  if (!IsRunning())
    return;

  timer_.Stop();
  SchedulePaint();
}

void WaybackMachineThrobber::OnPaint(gfx::Canvas* canvas) {
  if (!IsRunning())
    return;

  base::TimeDelta elapsed_time = base::TimeTicks::Now() - start_time_;
  gfx::PaintThrobberSpinning(
      canvas, GetContentsBounds(), SK_ColorWHITE, elapsed_time);
}

bool WaybackMachineThrobber::IsRunning() const {
  return timer_.IsRunning();
}

BEGIN_METADATA(WaybackMachineThrobber)
END_METADATA
