/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_DANCING_BOOKS_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_DANCING_BOOKS_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace speedreader {

class SpeedreaderDancingBooks : public views::View {
 public:
  METADATA_HEADER(SpeedreaderDancingBooks);
  SpeedreaderDancingBooks() = default;
  ~SpeedreaderDancingBooks() override = default;
  SpeedreaderDancingBooks& operator=(const SpeedreaderDancingBooks&) = delete;
  SpeedreaderDancingBooks(const SpeedreaderDancingBooks&) = delete;

 protected:
  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_DANCING_BOOKS_H_
