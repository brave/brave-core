/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_

#include <memory>

#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"

namespace tor {
class TorProfileService;
}  // namespace tor

class BraveOmniboxViewViews : public OmniboxViewViews,
                              public tor::TorLauncherServiceObserver {
 public:
  BraveOmniboxViewViews(OmniboxEditController* controller,
                        std::unique_ptr<OmniboxClient> client,
                        bool popup_window_mode,
                        LocationBarView* location_bar,
                        const gfx::FontList& font_list);
  ~BraveOmniboxViewViews() override;

 private:
  // tor::TorLauncherServiceObserver:
  void OnTorCircuitEstablished(bool result) override;
  void OnTorInitializing(const std::string& percentage) override;

  tor::TorProfileService* tor_profile_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveOmniboxViewViews);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
