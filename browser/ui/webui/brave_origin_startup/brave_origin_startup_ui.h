/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ORIGIN_STARTUP_BRAVE_ORIGIN_STARTUP_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ORIGIN_STARTUP_BRAVE_ORIGIN_STARTUP_UI_H_

#include "brave/components/brave_origin/buildflags/buildflags.h"

static_assert(BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED));

#include <memory>

#include "base/functional/callback.h"
#include "brave/components/brave_origin/mojom/brave_origin_startup.mojom.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

inline constexpr char kBraveOriginStartupURL[] =
    "chrome://brave-origin-startup/";
inline constexpr char kBraveOriginStartupHost[] = "brave-origin-startup";

class BraveOriginStartupHandler;

namespace content {
class BrowserContext;
class WebUI;
}  // namespace content

class BraveOriginStartupUI : public ui::MojoWebUIController {
 public:
  explicit BraveOriginStartupUI(content::WebUI* web_ui);
  ~BraveOriginStartupUI() override;

  BraveOriginStartupUI(const BraveOriginStartupUI&) = delete;
  BraveOriginStartupUI& operator=(const BraveOriginStartupUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<brave_origin::mojom::BraveOriginStartupHandler>
          receiver);

  using OpenBuyWindowCallback = base::RepeatingClosure;
  using CloseDialogCallback = base::OnceClosure;

  // Set callbacks from the view that hosts this WebUI. Called after
  // construction because the WebUI controller is framework-created, but the
  // view supplies the SKU browser context and close/buy callbacks once the
  // DOM is loaded. A pending_receiver_ bridges the gap if mojo binds first.
  // |sku_browser_context| is a regular profile's context for SKU service
  // (the system profile used for WebUI doesn't support SKU).
  void SetCallbacks(content::BrowserContext* sku_browser_context,
                    OpenBuyWindowCallback open_buy_window_callback,
                    CloseDialogCallback close_dialog_callback);

 private:
  void CreateHandler(content::BrowserContext* sku_browser_context,
                     OpenBuyWindowCallback open_buy_window_callback,
                     CloseDialogCallback close_dialog_callback);

  std::unique_ptr<BraveOriginStartupHandler> handler_;
  mojo::PendingReceiver<brave_origin::mojom::BraveOriginStartupHandler>
      pending_receiver_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveOriginStartupUIConfig
    : public content::DefaultWebUIConfig<BraveOriginStartupUI> {
 public:
  BraveOriginStartupUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBraveOriginStartupHost) {}
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ORIGIN_STARTUP_BRAVE_ORIGIN_STARTUP_UI_H_
