/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_DRAG_CONTEXT_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_DRAG_CONTEXT_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/point.h"

namespace views {
class View;
}  // namespace views

class SidebarItemDragContext final {
 public:
  static bool CanStartDrag(const gfx::Point press_pt, const gfx::Point p);

  SidebarItemDragContext();
  ~SidebarItemDragContext();
  SidebarItemDragContext(const SidebarItemDragContext&) = delete;
  SidebarItemDragContext& operator=(const SidebarItemDragContext&) = delete;

  bool ShouldMoveItem() const;

  std::optional<size_t> source_index() const { return source_index_; }
  void set_source_index(std::optional<size_t> index) { source_index_ = index; }

  void set_source(views::View* source) { source_ = source; }
  views::View* source() const { return source_; }

  void set_drag_indicator_index(std::optional<size_t> index) {
    drag_indicator_index_ = index;
  }
  size_t GetTargetIndex() const;

  void Reset();

 private:
  std::optional<size_t> source_index_ = std::nullopt;
  raw_ptr<views::View> source_ = nullptr;
  std::optional<size_t> drag_indicator_index_ = std::nullopt;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_DRAG_CONTEXT_H_
