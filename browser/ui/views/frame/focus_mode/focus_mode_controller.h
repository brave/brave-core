/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/ui/views/frame/focus_mode/focus_mode_types.h"

class FocusModeRevealZone;
class FocusModeRevealedLock;

namespace views {
class Widget;
}

// Orchestrates Focus Mode for a single browser window. Owns the enabled state,
// the set of FocusModeRevealZone instances (one per edge), and delegates layout
// and visibility changes.
class FocusModeController {
 public:
  enum class Edge { kTop, kLeft, kRight };

  class Delegate {
   public:
    virtual void OnFocusModeChanged(bool enabled) = 0;
    virtual void OnFocusModeRevealFractionChanged(Edge edge,
                                                  double fraction) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  FocusModeController(Delegate* delegate, views::Widget* widget);
  FocusModeController(const FocusModeController&) = delete;
  FocusModeController& operator=(const FocusModeController&) = delete;
  ~FocusModeController();

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  // True if the given edge's zone is in any state other than kClosed.
  bool IsRevealed(Edge edge) const;

  // Current visibility fraction (0 = hidden, 1 = fully visible) for an edge.
  double GetVisibleFraction(Edge edge) const;

  // Returns an RAII lock that keeps the given edge's chrome revealed while
  // alive. Returns nullptr if focus mode is not enabled.
  [[nodiscard]] std::unique_ptr<FocusModeRevealedLock> GetRevealedLock(
      Edge edge,
      FocusModeTransition transition);

 private:
  void OnRevealFractionChanged(Edge edge, double fraction);
  FocusModeRevealZone* GetZone(Edge edge) const;

  raw_ref<Delegate> delegate_;
  raw_ref<views::Widget> widget_;
  bool enabled_ = false;
  base::flat_map<Edge, std::unique_ptr<FocusModeRevealZone>> zones_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_FOCUS_MODE_CONTROLLER_H_
