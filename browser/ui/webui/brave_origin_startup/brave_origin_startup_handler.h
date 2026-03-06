/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_ORIGIN_STARTUP_BRAVE_ORIGIN_STARTUP_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_ORIGIN_STARTUP_BRAVE_ORIGIN_STARTUP_HANDLER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_origin/mojom/brave_origin_startup.mojom.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace content {
class BrowserContext;
}  // namespace content

class BraveOriginStartupHandler
    : public brave_origin::mojom::BraveOriginStartupHandler {
 public:
  using OpenBuyWindowCallback = base::RepeatingClosure;
  using CloseDialogCallback = base::OnceClosure;

  BraveOriginStartupHandler(content::BrowserContext* browser_context,
                            PrefService* local_state,
                            OpenBuyWindowCallback open_buy_window_callback,
                            CloseDialogCallback close_dialog_callback);
  ~BraveOriginStartupHandler() override;

  BraveOriginStartupHandler(const BraveOriginStartupHandler&) = delete;
  BraveOriginStartupHandler& operator=(const BraveOriginStartupHandler&) =
      delete;

  void BindInterface(
      mojo::PendingReceiver<brave_origin::mojom::BraveOriginStartupHandler>
          receiver);

  // brave_origin::mojom::BraveOriginStartupHandler:
  void CheckPurchaseState(CheckPurchaseStateCallback callback) override;
  void VerifyPurchaseId(const std::string& purchase_id,
                        VerifyPurchaseIdCallback callback) override;
  void GetBuyUrl(GetBuyUrlCallback callback) override;
  void OpenBuyWindow() override;
  void CloseDialog() override;

 private:
  bool EnsureSkusConnected();
  void OnCredentialSummary(CheckPurchaseStateCallback callback,
                           skus::mojom::SkusResultPtr summary);
  void OnRefreshOrder(const std::string& order_id,
                      VerifyPurchaseIdCallback callback,
                      skus::mojom::SkusResultPtr result);
  void OnFetchOrderCredentials(VerifyPurchaseIdCallback callback,
                               skus::mojom::SkusResultPtr result);
  void OnVerifyCredentialSummary(VerifyPurchaseIdCallback callback,
                                 skus::mojom::SkusResultPtr summary);

  OpenBuyWindowCallback open_buy_window_callback_;
  CloseDialogCallback close_dialog_callback_;

  std::string origin_sku_domain_;

  raw_ptr<content::BrowserContext> browser_context_;
  raw_ptr<PrefService> local_state_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  mojo::Receiver<brave_origin::mojom::BraveOriginStartupHandler> receiver_{
      this};

  base::WeakPtrFactory<BraveOriginStartupHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_ORIGIN_STARTUP_BRAVE_ORIGIN_STARTUP_HANDLER_H_
