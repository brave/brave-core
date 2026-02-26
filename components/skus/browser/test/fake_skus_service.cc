/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/test/fake_skus_service.h"

#include <utility>

namespace skus {

FakeSkusService::FakeSkusService() = default;
FakeSkusService::~FakeSkusService() = default;

mojo::PendingRemote<skus::mojom::SkusService> FakeSkusService::MakeRemote() {
  receiver_.reset();
  mojo::PendingRemote<skus::mojom::SkusService> remote;
  receiver_.Bind(remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void FakeSkusService::SetCredentialSummaryResponse(
    const std::string& response) {
  credential_summary_response_ = response;
}

void FakeSkusService::RefreshOrder(const std::string& domain,
                                   const std::string& order_id,
                                   RefreshOrderCallback callback) {
  std::move(callback).Run(
      skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok, ""));
}

void FakeSkusService::FetchOrderCredentials(
    const std::string& domain,
    const std::string& order_id,
    FetchOrderCredentialsCallback callback) {
  std::move(callback).Run(
      skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok, ""));
}

void FakeSkusService::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    PrepareCredentialsPresentationCallback callback) {
  std::move(callback).Run(
      skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok, ""));
}

void FakeSkusService::CredentialSummary(const std::string& domain,
                                        CredentialSummaryCallback callback) {
  std::move(callback).Run(skus::mojom::SkusResult::New(
      skus::mojom::SkusResultCode::Ok, credential_summary_response_));
}

void FakeSkusService::SubmitReceipt(const std::string& domain,
                                    const std::string& order_id,
                                    const std::string& receipt,
                                    SubmitReceiptCallback callback) {
  std::move(callback).Run(
      skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok, ""));
}

void FakeSkusService::CreateOrderFromReceipt(
    const std::string& domain,
    const std::string& receipt,
    CreateOrderFromReceiptCallback callback) {
  std::move(callback).Run(
      skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok, ""));
}

}  // namespace skus
