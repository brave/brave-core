/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"
#include "components/keyed_service/core/keyed_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class BraveVpnService : public KeyedService,
                        public brave_vpn::BraveVPNOSConnectionAPI::Observer {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConnectionStateChanged(bool connected) = 0;
    virtual void OnConnectionCreated() = 0;
    virtual void OnConnectionRemoved() = 0;

   protected:
    ~Observer() override = default;
  };

  explicit BraveVpnService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveVpnService() override;

  BraveVpnService(const BraveVpnService&) = delete;
  BraveVpnService& operator=(const BraveVpnService&) = delete;

  // KeyedService overrides:
  void Shutdown() override;

  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;

  void GetAllServerRegions(ResponseCallback callback);
  void GetTimezonesForRegions(ResponseCallback callback);
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region);
  void GetSubscriberCredential(ResponseCallback callback,
                               const std::string& product_type,
                               const std::string& product_id,
                               const std::string& validation_method,
                               const std::string& purchase_token);
  void VerifyPurchaseToken(ResponseCallback callback,
                           const std::string& purchase_token,
                           const std::string& product_id,
                           const std::string& product_type);

  void Connect();
  void Disconnect();
  bool IsConnected() const;
  void CreateVPNConnection();
  void RemoveVPNConnnection();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  using URLRequestCallback =
      base::OnceCallback<void(int,
                              const std::string&,
                              const base::flat_map<std::string, std::string>&)>;

  // brave_vpn::BraveVPNOSConnectionAPI::Observer overrides:
  void OnCreated(const std::string& name) override;
  void OnRemoved(const std::string& name) override;
  void OnConnected(const std::string& name) override;
  void OnDisconnected(const std::string& name) override;

  void OAuthRequest(const GURL& url,
                    const std::string& method,
                    const std::string& post_data,
                    bool set_app_ident,
                    URLRequestCallback callback);

  void OnGetResponse(ResponseCallback callback,
                     int status,
                     const std::string& body,
                     const base::flat_map<std::string, std::string>& headers);

  void OnGetSubscriberCredential(
      ResponseCallback callback,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  brave_vpn::BraveVPNConnectionInfo GetConnectionInfo();

  bool is_connected_ = false;
  base::ObserverList<Observer> observers_;
  base::ScopedObservation<brave_vpn::BraveVPNOSConnectionAPI,
                          brave_vpn::BraveVPNOSConnectionAPI::Observer>
      observed_{this};
  api_request_helper::APIRequestHelper api_request_helper_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_H_
