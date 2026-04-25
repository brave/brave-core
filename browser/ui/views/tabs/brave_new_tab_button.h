/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/size.h"

class TabStrip;
namespace views {
class ButtonListener;
class MenuRunner;
}  // namespace views

namespace containers {
class ContainersBrowserTest;
class ContainersMenuModel;
}  // namespace containers

class BraveNewTabButton : public NewTabButton {
  // Note that NewTabButton is missing METADATA_HEADER, so we need to declare
  // TabStripControlButton as paren.
  METADATA_HEADER(BraveNewTabButton, TabStripControlButton)

 public:
  // This static members are shared with BraveTabSearchButton
  // TODO(sko) If we could make TabSearchButton inherit BraveNewTabButton,
  // we might not need to do this any more.
  static gfx::Size GetButtonSize();

  BraveNewTabButton(PressedCallback callback,
                    const gfx::VectorIcon& icon /* this won't be used */,
                    Edge fixed_flat_edge,
                    Edge animated_flat_edge,
                    BrowserWindowInterface* browser_window_interface);
  BraveNewTabButton(PressedCallback callback,
                    const gfx::VectorIcon& icon /* this won't be used */,
                    BrowserWindowInterface* browser_window_interface);
  ~BraveNewTabButton() override;

#if BUILDFLAG(ENABLE_CONTAINERS)
  // NewTabButton:
  void ShowContextMenuForViewImpl(
      views::View* source,
      const gfx::Point& point,
      ui::mojom::MenuSourceType source_type) override;
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

 protected:
  // NewTabButton:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
#if BUILDFLAG(ENABLE_CONTAINERS)
  friend class containers::ContainersBrowserTest;

  class NewTabButtonContainersMenuDelegate;

  std::unique_ptr<NewTabButtonContainersMenuDelegate> containers_menu_delegate_;
  std::unique_ptr<containers::ContainersMenuModel> containers_menu_model_;
  std::unique_ptr<views::MenuRunner> containers_context_menu_runner_;

  // When true, the containers menu model and runner are created but RunMenuAt
  // is skipped so tests do not block in MenuRunner's nested loop.
  bool skip_containers_context_menu_runner_for_testing_ = false;
#endif

  const raw_ref<BrowserWindowInterface> browser_window_interface_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_NEW_TAB_BUTTON_H_
