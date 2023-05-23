// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SKUS_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SKUS_INTERNALS_UI_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/skus/common/skus_internals.mojom.h"
#include "content/public/browser/web_ui_controller.h"

class PrefService;

class SkusInternalsUI : public content::WebUIController,
                        public skus::mojom::SkusInternals {
 public:
  SkusInternalsUI(content::WebUI* web_ui, const std::string& host);
  ~SkusInternalsUI() override;
  SkusInternalsUI(const SkusInternalsUI&) = delete;
  SkusInternalsUI& operator=(const SkusInternalsUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<skus::mojom::SkusInternals> pending_receiver);

 private:
  // skus::mojom::SkusInternals overrides:
  void GetEventLog(GetEventLogCallback callback) override;
  void GetSkusState(GetSkusStateCallback callback) override;
  void ResetSkusState() override;

  raw_ptr<PrefService> local_state_ = nullptr;
  mojo::Receiver<skus::mojom::SkusInternals> skus_internals_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SKUS_INTERNALS_UI_H_
