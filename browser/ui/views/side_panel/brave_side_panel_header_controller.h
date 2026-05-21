/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_HEADER_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_HEADER_CONTROLLER_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_header.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "url/gurl.h"

class BrowserWindowInterface;

namespace views {
class ImageButton;
class Label;
}  // namespace views

class BraveSidePanelHeaderController : public BraveSidePanelHeader::Delegate {
 public:
  BraveSidePanelHeaderController(BrowserWindowInterface& browser_window,
                                 SidePanelEntry* entry);
  BraveSidePanelHeaderController(const BraveSidePanelHeaderController&) =
      delete;
  BraveSidePanelHeaderController& operator=(
      const BraveSidePanelHeaderController&) = delete;
  ~BraveSidePanelHeaderController() override;

  // BraveSidePanelHeader::Delegate:
  std::unique_ptr<views::Label> CreatePanelTitle() override;
  std::unique_ptr<views::ImageButton> CreateLaunchButton() override;
  std::unique_ptr<views::ImageButton> CreateCloseButton() override;

 private:
  void OnLaunchButtonPressed(const GURL& url);
  void OnCloseButtonPressed();

  const raw_ref<BrowserWindowInterface> browser_window_;
  base::WeakPtr<SidePanelEntry> entry_;
  base::WeakPtrFactory<BraveSidePanelHeaderController> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_HEADER_CONTROLLER_H_
