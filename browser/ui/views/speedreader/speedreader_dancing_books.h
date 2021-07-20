/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_DANCING_BOOKS_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_DANCING_BOOKS_H_

#include <utility>

#include "ui/gfx/geometry/vector2d.h"
#include "ui/native_theme/themed_vector_icon.h"
#include "ui/views/view.h"

namespace speedreader {

using BookGraphic = std::pair<gfx::Vector2d, const gfx::VectorIcon*>;

class SpeedreaderDancingBooks : public views::View {
 public:
  SpeedreaderDancingBooks();
  ~SpeedreaderDancingBooks() override;

 protected:
  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;

 private:
  std::array<BookGraphic, 6> graphics_locations_;
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_DANCING_BOOKS_H_
