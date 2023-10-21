/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/public/cpp/third_party_service.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "brave/components/services/brave_wallet/public/cpp/third_party_service_launcher.h"
#include "brave/components/services/brave_wallet/public/mojom/filecoin_utility.mojom.h"
#include "build/build_config.h"

namespace brave_wallet {

namespace {

constexpr base::TimeDelta kServiceProcessIdleTimeout{base::Seconds(5)};

template <typename T, typename V>
class Request : public base::RefCounted<Request<T, V>> {
 public:
  explicit Request(ThirdPartyService::ResultCallback<V> callback)
      : callback_(std::move(callback)) {}

  Request(const Request&) = delete;
  Request& operator=(const Request&) = delete;

  mojo::Remote<T>& remote() { return remote_; }
  ThirdPartyService::ResultCallback<V>& callback() { return callback_; }

  // Creates a pipe and binds it to the remote(), and sets up the
  // disconnect handler to invoke callback() with an error.
  mojo::PendingReceiver<T> BindRemote() {
    auto receiver = remote_.BindNewPipeAndPassReceiver();
    remote_.set_disconnect_handler(
        base::BindOnce(&Request::OnRemoteDisconnected, this));
    return receiver;
  }

  void OnResult(const absl::optional<V>& result) {
    if (!callback()) {
      return;
    }

    // Copy the callback onto the stack before resetting the Remote, as that may
    // delete |this|.
    auto local_callback = std::move(callback());

    // Reset the |remote_| since we aren't using it again and we don't want it
    // to trip the disconnect handler. May delete |this|.
    remote_.reset();

    // We run the callback after reset just in case it does anything funky like
    // spin a nested RunLoop.
    std::move(local_callback).Run(std::move(result));
  }

 private:
  friend class base::RefCounted<Request>;

  ~Request() = default;

  void OnRemoteDisconnected() {
    if (callback()) {
      std::move(callback()).Run(absl::nullopt);
    }
  }

  mojo::Remote<T> remote_;
  ThirdPartyService::ResultCallback<V> callback_;
};

}  // namespace

// static
ThirdPartyService& ThirdPartyService::Get() {
  static base::NoDestructor<ThirdPartyService> service;
  return *service;
}

void ThirdPartyService::ResetForTesting() {
  service_.reset();
}

ThirdPartyService::ThirdPartyService() = default;
ThirdPartyService::~ThirdPartyService() = default;

void ThirdPartyService::BindRemote() {
  if (service_.is_bound()) {
    return;
  }

  ThirdPartyServiceLauncher::GetInstance()->Launch(
      service_.BindNewPipeAndPassReceiver());

  service_.reset_on_disconnect();
  service_.reset_on_idle_timeout(kServiceProcessIdleTimeout);
}

void ThirdPartyService::BLSPrivateKeyToPublicKey(
    const std::vector<uint8_t>& private_key,
    BLSPrivateKeyToPublicKeyCallback callback) {
  BindRemote();
  auto request =
      base::MakeRefCounted<Request<third_party_service::mojom::FilecoinUtility,
                                   std::vector<uint8_t>>>(std::move(callback));
  service_->BindFilecoinUtility(request->BindRemote());
  request->remote()->BLSPrivateKeyToPublicKey(
      private_key,
      base::BindOnce(&Request<third_party_service::mojom::FilecoinUtility,
                              std::vector<uint8_t>>::OnResult,
                     request));
}

void ThirdPartyService::SignFilecoinTransaction(
    bool is_mainnet,
    const std::string& transaction,
    const std::vector<uint8_t>& private_key,
    TransactionSignCallback callback) {
  BindRemote();
  auto request = base::MakeRefCounted<
      Request<third_party_service::mojom::FilecoinUtility, std::string>>(
      std::move(callback));
  service_->BindFilecoinUtility(request->BindRemote());
  request->remote()->TransactionSign(
      is_mainnet, transaction, private_key,
      base::BindOnce(&Request<third_party_service::mojom::FilecoinUtility,
                              std::string>::OnResult,
                     request));
}

}  // namespace brave_wallet
