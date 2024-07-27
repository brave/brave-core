/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_UI_H_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/common/mojom/rewards_page.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_rewards {

// The WebUI controller for the Rewards page when viewed in a tab.
class RewardsPageUI : public content::WebUIController,
                      public mojom::RewardsPageHandlerFactory {
 public:
  explicit RewardsPageUI(content::WebUI* web_ui, const std::string& host);
  ~RewardsPageUI() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::RewardsPageHandlerFactory> receiver);

 private:
  // mojom::RewardsPageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<mojom::RewardsPage> page,
      mojo::PendingReceiver<mojom::RewardsPageHandler> handler) override;

  std::unique_ptr<mojom::RewardsPageHandler> handler_;
  mojo::Receiver<RewardsPageHandlerFactory> factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_UI_H_
