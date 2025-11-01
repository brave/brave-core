// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NEW_TAB_TAKEOVER_UI_HANDLER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NEW_TAB_TAKEOVER_UI_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace ntp_background_images {
class NTPBackgroundImagesService;
}  // namespace ntp_background_images

// TODO(aseren): Move part of this functionality to separate class to share with
// Android.
class NewTabTakeoverUIHandler final
    : public new_tab_takeover::mojom::NewTabTakeover {
 public:
  NewTabTakeoverUIHandler(ntp_background_images::NTPBackgroundImagesService*
                              ntp_background_images_service);

  NewTabTakeoverUIHandler(const NewTabTakeoverUIHandler&) = delete;
  NewTabTakeoverUIHandler& operator=(const NewTabTakeoverUIHandler&) = delete;

  ~NewTabTakeoverUIHandler() override;

  void BindInterface(
      mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover>
          pending_receiver);

 private:
  // new_tab_takeover::mojom::NewTabTakeover:
  void SetSponsoredRichMediaAdEventHandler(
      mojo::PendingReceiver<
          ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
          event_handler) override;
  void GetCurrentWallpaper(const std::string& creative_instance_id,
                           GetCurrentWallpaperCallback callback) override;
  void NavigateToUrl(const GURL& url) override;

  mojo::Receiver<new_tab_takeover::mojom::NewTabTakeover> receiver_{this};

  const raw_ptr<ntp_background_images::NTPBackgroundImagesService>
      ntp_background_images_service_;  // Not owned.

  base::WeakPtrFactory<NewTabTakeoverUIHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NEW_TAB_TAKEOVER_UI_HANDLER_H_
