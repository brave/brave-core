/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_PAGE_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_PAGE_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_account {
class BraveAccountService;
}

class BraveAccountPageHandler : public brave_account::mojom::PageHandler {
 public:
  BraveAccountPageHandler(
      brave_account::BraveAccountService* brave_account_service,
      mojo::PendingReceiver<brave_account::mojom::PageHandler>
          pending_receiver);

  BraveAccountPageHandler(const BraveAccountPageHandler&) = delete;
  BraveAccountPageHandler& operator=(const BraveAccountPageHandler&) = delete;

  ~BraveAccountPageHandler() override;

  // brave_account::mojom::PageHandler:
  void RegisterInitialize(const std::string& email,
                          const std::string& blinded_message,
                          RegisterInitializeCallback callback) override;

  void RegisterFinalize(const std::string& encrypted_verification_token,
                        const std::string& serialized_record,
                        RegisterFinalizeCallback callback) override;

 private:
  const raw_ptr<brave_account::BraveAccountService> brave_account_service_;
  mojo::Receiver<brave_account::mojom::PageHandler> receiver_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_PAGE_HANDLER_H_
