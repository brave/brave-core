/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_layout_provider.h"

#include "ui/views/layout/layout_provider.h"

int BraveLayoutProvider::GetCornerRadiusMetric(
    views::EmphasisMetric emphasis_metric,
    const gfx::Size& size) const {
  switch (emphasis_metric) {
    case views::EMPHASIS_NONE:
      return 0;
    case views::EMPHASIS_LOW:
    case views::EMPHASIS_MEDIUM:
      return 2;
    case views::EMPHASIS_HIGH:
    case views::EMPHASIS_MAXIMUM:
      return 4;
  }
}
