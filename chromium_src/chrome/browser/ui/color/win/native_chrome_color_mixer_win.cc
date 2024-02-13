/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/callback_list.h"
#include "base/no_destructor.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/win/accent_color_observer.h"

namespace ui {

class FakeAccentColorObserver {
 public:
  static FakeAccentColorObserver* Get() {
    static base::NoDestructor<FakeAccentColorObserver> observer;
    return observer.get();
  }

  FakeAccentColorObserver() {}
  FakeAccentColorObserver(const FakeAccentColorObserver&) = delete;
  FakeAccentColorObserver& operator=(const FakeAccentColorObserver&) = delete;
  ~FakeAccentColorObserver() {}

  base::CallbackListSubscription Subscribe(base::RepeatingClosure callback) {
    return callbacks_.Add(std::move(callback));
  }

  std::optional<SkColor> accent_color() const { return std::nullopt; }
  std::optional<SkColor> accent_color_inactive() const { return std::nullopt; }
  std::optional<SkColor> accent_border_color() const { return std::nullopt; }
  bool use_dwm_frame_color() const { return false; }

 private:
  base::RepeatingClosureList callbacks_;
};

}  // namespace ui

#define AccentColorObserver FakeAccentColorObserver
#include "src/chrome/browser/ui/color/win/native_chrome_color_mixer_win.cc"
#undef AccentColorObserver
