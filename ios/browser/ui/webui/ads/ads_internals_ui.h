// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_ADS_ADS_INTERNALS_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_ADS_ADS_INTERNALS_UI_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/browser/internals/ads_internals_handler.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "url/gurl.h"

// MARK: - BASED ON: brave/browser/ui/webui/ads_internals/ads_internals_ui.h

namespace web {
class WebUIIOS;
}  // namespace web

class AdsInternalsUI : public web::WebUIIOSController {
 public:
  AdsInternalsUI(web::WebUIIOS* const web_ui, const GURL& url);

  AdsInternalsUI(const AdsInternalsUI&) = delete;
  AdsInternalsUI& operator=(const AdsInternalsUI&) = delete;

  ~AdsInternalsUI() override;

  void BindInterface(
      mojo::PendingReceiver<bat_ads::mojom::AdsInternals> pending_receiver);

 private:
  AdsInternalsHandler handler_;

  base::WeakPtrFactory<AdsInternalsUI> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_ADS_ADS_INTERNALS_UI_H_
