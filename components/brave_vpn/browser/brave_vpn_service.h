/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

#if !BUILDFLAG(IS_ANDROID)
class BraveAppMenuBrowserTest;
class BraveAppMenuModelBrowserTest;
class BraveBrowserCommandControllerTest;
#endif  // !BUILDFLAG(IS_ANDROID)

namespace brave_vpn {

// Base class for Brave VPN keyed service, which implements the basic mojo
// interface and observer management. It also declares virtual functions used by
// external code. The implementation is in BraveVpnServiceImpl, which is
// different for Architecture 1.0 and Architecture 2.0.

// This class is used by desktop and Android. However, it includes desktop
// specific implementations, and it's hidden by IS_ANDROID buildflag.

class BraveVpnService : public mojom::ServiceHandler, public KeyedService {
 public:
  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;
#if !BUILDFLAG(IS_ANDROID)
  using mojom::ServiceHandler::GetConnectionState;
#endif  // !BUILDFLAG(IS_ANDROID)

  BraveVpnService();
  ~BraveVpnService() override;

  BraveVpnService(const BraveVpnService&) = delete;
  BraveVpnService& operator=(const BraveVpnService&) = delete;

  // mojom::ServiceHandler overrides:
  void AddObserver(mojo::PendingRemote<mojom::ServiceObserver> observer) final;

  // Public interface exposed to other components.
  virtual bool IsBraveVPNEnabled() const = 0;
  virtual bool IsPurchased() const = 0;
  virtual void ReloadPurchasedState() = 0;
  virtual std::string GetCurrentEnvironment() const = 0;
#if !BUILDFLAG(IS_ANDROID)
  // Public interface for desktop components.
  virtual bool IsConnected() const = 0;
  virtual void ToggleConnection() = 0;
  virtual mojom::ConnectionState GetConnectionState() const = 0;
  virtual void RecordWidgetUsageMetrics(bool new_usage) = 0;
#else   //  !BUILDFLAG(IS_ANDROID)
  // Public interface for Android native worker.
  virtual void GetTimezonesForRegions(ResponseCallback callback) = 0;
  virtual void GetHostnamesForRegion(ResponseCallback callback,
                                     const std::string& region,
                                     const std::string& region_precision) = 0;
  virtual void GetProfileCredentials(ResponseCallback callback,
                                     const std::string& subscriber_credential,
                                     const std::string& hostname) = 0;
  virtual void GetWireguardProfileCredentials(
      ResponseCallback callback,
      const std::string& subscriber_credential,
      const std::string& public_key,
      const std::string& hostname) = 0;
  virtual void VerifyCredentials(ResponseCallback callback,
                                 const std::string& hostname,
                                 const std::string& client_id,
                                 const std::string& subscriber_credential,
                                 const std::string& api_auth_token) = 0;
  virtual void InvalidateCredentials(ResponseCallback callback,
                                     const std::string& hostname,
                                     const std::string& client_id,
                                     const std::string& subscriber_credential,
                                     const std::string& api_auth_token) = 0;
  virtual void VerifyPurchaseToken(ResponseCallback callback,
                                   const std::string& purchase_token,
                                   const std::string& product_id,
                                   const std::string& product_type,
                                   const std::string& bundle_id) = 0;
  virtual void GetSubscriberCredential(ResponseCallback callback,
                                       const std::string& product_type,
                                       const std::string& product_id,
                                       const std::string& validation_method,
                                       const std::string& purchase_token,
                                       const std::string& bundle_id) = 0;
  virtual void GetSubscriberCredentialV12(ResponseCallback callback) = 0;
  virtual void RecordAllMetrics() = 0;
  virtual void RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                          int64_t session_end_time_ms) = 0;
#endif  // !BUILDFLAG(IS_ANDROID)

  void BindInterface(mojo::PendingReceiver<mojom::ServiceHandler> receiver);
#if BUILDFLAG(IS_ANDROID)
  mojo::PendingRemote<brave_vpn::mojom::ServiceHandler> MakeRemote();
#endif  // BUILDFLAG(IS_ANDROID)

 protected:
  // KeyedService overrides:
  void Shutdown() override;

  // Observer notification functions.
  void NotifyPurchasedStateChanged(
      mojom::PurchasedState state,
      const std::optional<std::string>& description);
#if !BUILDFLAG(IS_ANDROID)
  void NotifyConnectionStateChanged(mojom::ConnectionState state);
  void NotifySelectedRegionChanged(mojom::RegionPtr region);
  void NotifySmartProxyRoutingStateChanged(bool enabled);
#endif  // !BUILDFLAG(IS_ANDROID)

  SEQUENCE_CHECKER(sequence_checker_);

 private:
  friend class BraveVpnButtonUnitTest;
#if !BUILDFLAG(IS_ANDROID)
  friend class ::BraveAppMenuBrowserTest;
  friend class ::BraveAppMenuModelBrowserTest;
  friend class ::BraveBrowserCommandControllerTest;
#endif  // !BUILDFLAG(IS_ANDROID)

#if !BUILDFLAG(IS_ANDROID)
  virtual void SetConnectionStateForTesting(mojom::ConnectionState state) = 0;
  virtual void SetPurchasedStateForTesting(const std::string& env,
                                           mojom::PurchasedState state) = 0;
#endif  // !BUILDFLAG(IS_ANDROID)

  mojo::RemoteSet<mojom::ServiceObserver> observers_;
  mojo::ReceiverSet<mojom::ServiceHandler> receivers_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_BRAVE_VPN_SERVICE_H_
