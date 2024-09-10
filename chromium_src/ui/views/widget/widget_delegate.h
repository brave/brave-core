/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_WIDGET_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_WIDGET_DELEGATE_H_

// Adds interfaces for overriding desired bounds' position. These are used by
// constrained window views. We need separate method from the existing
// desired_bounds_delegate() so that we can override position in the middle of
// desired_bounds_delegate() without falling into infinite recursion.
#define set_desired_bounds_delegate                                 \
  set_desired_bounds_delegate_unused();                             \
  gfx::Point get_desired_position() {                               \
    return desired_position_delegate_.Run();                        \
  }                                                                 \
  bool has_desired_position_delegate() const {                      \
    return static_cast<bool>(desired_position_delegate_);           \
  }                                                                 \
  void set_desired_position_delegate(                               \
      base::RepeatingCallback<gfx::Point()> delegate) {             \
    desired_position_delegate_ = std::move(delegate);               \
  }                                                                 \
  base::WeakPtr<WidgetDelegate> GetWeakPtr() {                      \
    return weak_ptr_factory_.GetWeakPtr();                          \
  }                                                                 \
                                                                    \
 private:                                                           \
  base::RepeatingCallback<gfx::Point()> desired_position_delegate_; \
                                                                    \
 public:                                                            \
  void set_desired_bounds_delegate

#include "src/ui/views/widget/widget_delegate.h"  // IWYU pragma: export

#undef set_desired_bounds_delegate

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_WIDGET_DELEGATE_H_
