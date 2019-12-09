/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_close_button.h"

#include <algorithm>
#include <utility>

#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace {

int GetRadius(const gfx::Size& size) {
  return std::min(size.width(), size.height()) / 2;
}

class TabCloseButtonHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  TabCloseButtonHighlightPathGenerator() = default;

  TabCloseButtonHighlightPathGenerator(
      const TabCloseButtonHighlightPathGenerator&) = delete;
  TabCloseButtonHighlightPathGenerator& operator=(
      const TabCloseButtonHighlightPathGenerator&) = delete;

  // views::HighlightPathGenerator:
  SkPath GetHighlightPath(const views::View* view) override {
    const gfx::Rect bounds = view->GetContentsBounds();
    const gfx::Point center = bounds.CenterPoint();
    return SkPath().addCircle(center.x(), center.y(), GetRadius(bounds.size()));
  }
};

}  //  namespace

BraveTabCloseButton::BraveTabCloseButton(
    views::ButtonListener* listener,
    MouseEventCallback mouse_event_callback)
    : TabCloseButton(listener, std::move(mouse_event_callback)) {
  views::HighlightPathGenerator::Install(
      this, std::make_unique<TabCloseButtonHighlightPathGenerator>());
}

std::unique_ptr<views::InkDropMask>
BraveTabCloseButton::CreateInkDropMask() const {
  const gfx::Rect bounds = GetContentsBounds();
  return std::make_unique<views::CircleInkDropMask>(
      size(), GetMirroredRect(bounds).CenterPoint(), GetRadius(bounds.size()));
}
