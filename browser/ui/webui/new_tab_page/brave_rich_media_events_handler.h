/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_RICH_MEDIA_EVENTS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_RICH_MEDIA_EVENTS_HANDLER_H_

#include <memory>

#include "brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace ntp_background_images {
class NTPP3AHelper;
}  // namespace ntp_background_images

// TODO(tmancey): @aseren why is these called brave_rich_media_events_handler
// and html5_ntt_ui as both are related but use different names. Should this not
// be inside a brave_ads folder like other handlers?
class BraveRichMediaEventsHandler
    : public brave_new_tab_page::mojom::RichMediaEventsHandler {
 public:
  explicit BraveRichMediaEventsHandler(
      brave_ads::AdsService* ads_service,
      std::unique_ptr<ntp_background_images::NTPP3AHelper> ntp_p3a_helper,
      mojo::PendingReceiver<brave_new_tab_page::mojom::RichMediaEventsHandler>
          pending_receiver);

  BraveRichMediaEventsHandler(const BraveRichMediaEventsHandler&) = delete;
  BraveRichMediaEventsHandler& operator=(const BraveRichMediaEventsHandler&) = delete;

  ~BraveRichMediaEventsHandler() override;

  // brave_new_tab_page::mojom::RichMediaEventsHandler:
  void ReportRichMediaEvent(const std::string& placement_id,
                            const std::string& creative_instance_id,
                            const std::string& event_type) override;

 private:
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;  // Not owned.
  std::unique_ptr<ntp_background_images::NTPP3AHelper> ntp_p3a_helper_;
  mojo::Receiver<brave_new_tab_page::mojom::RichMediaEventsHandler> receiver_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_RICH_MEDIA_EVENTS_HANDLER_H_
