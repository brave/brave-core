// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_COORDINATOR_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

class Browser;
class SidePanelRegistry;

namespace views {
class Label;
class View;
}  // namespace views

class MobileViewSidePanelCoordinator : public content::WebContentsObserver {
 public:
  MobileViewSidePanelCoordinator(Browser& browser,
                                 SidePanelRegistry& global_registry,
                                 const GURL& url);
  ~MobileViewSidePanelCoordinator() override;
  MobileViewSidePanelCoordinator(const MobileViewSidePanelCoordinator&) =
      delete;
  MobileViewSidePanelCoordinator& operator=(
      const MobileViewSidePanelCoordinator&) = delete;

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;

 private:
  std::unique_ptr<views::View> CreateView();
  SidePanelEntry::Key GetEntryKey() const;
  void DeregisterEntry();

  raw_ref<Browser> browser_;
  raw_ref<SidePanelRegistry> global_registry_;
  raw_ptr<views::Label> label_ = nullptr;
  const GURL url_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_MOBILE_VIEW_MOBILE_VIEW_SIDE_PANEL_COORDINATOR_H_
