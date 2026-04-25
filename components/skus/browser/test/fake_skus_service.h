/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_TEST_FAKE_SKUS_SERVICE_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_TEST_FAKE_SKUS_SERVICE_H_

#include <string>

#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace skus {

// Fake SkusService that returns a configurable CredentialSummary response.
// Used by tests to exercise the real JSON parsing logic in
// OnCredentialSummary without a network-backed SKU service.
class FakeSkusService : public skus::mojom::SkusService {
 public:
  FakeSkusService();
  ~FakeSkusService() override;

  FakeSkusService(const FakeSkusService&) = delete;
  FakeSkusService& operator=(const FakeSkusService&) = delete;

  // Creates a new PendingRemote bound to this fake. Safe to call multiple
  // times (resets the previous binding).
  mojo::PendingRemote<skus::mojom::SkusService> MakeRemote();

  // Sets the JSON string that CredentialSummary will return.
  void SetCredentialSummaryResponse(const std::string& response);

  // skus::mojom::SkusService:
  void RefreshOrder(const std::string& domain,
                    const std::string& order_id,
                    RefreshOrderCallback callback) override;
  void FetchOrderCredentials(const std::string& domain,
                             const std::string& order_id,
                             FetchOrderCredentialsCallback callback) override;
  void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& path,
      PrepareCredentialsPresentationCallback callback) override;
  void CredentialSummary(const std::string& domain,
                         CredentialSummaryCallback callback) override;
  void SubmitReceipt(const std::string& domain,
                     const std::string& order_id,
                     const std::string& receipt,
                     SubmitReceiptCallback callback) override;
  void CreateOrderFromReceipt(const std::string& domain,
                              const std::string& receipt,
                              CreateOrderFromReceiptCallback callback) override;

 private:
  std::string credential_summary_response_;
  mojo::Receiver<skus::mojom::SkusService> receiver_{this};
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_TEST_FAKE_SKUS_SERVICE_H_
