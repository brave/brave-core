/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_

#include <string>

#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_vpn/brave_vpn.mojom-shared.h"
#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/brave_vpn_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

typedef brave_vpn::mojom::ConnectionState ConnectionState;

class BraveVpnServiceDesktop
    : public BraveVpnService,
      public brave_vpn::BraveVPNOSConnectionAPI::Observer,
      public brave_vpn::mojom::ServiceHandler {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConnectionStateChanged(ConnectionState state) = 0;
    virtual void OnConnectionCreated() = 0;
    virtual void OnConnectionRemoved() = 0;

   protected:
    ~Observer() override = default;
  };

  explicit BraveVpnServiceDesktop(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveVpnServiceDesktop() override;

  BraveVpnServiceDesktop(const BraveVpnServiceDesktop&) = delete;
  BraveVpnServiceDesktop& operator=(const BraveVpnServiceDesktop&) = delete;

  void RemoveVPNConnnection();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  bool is_connected() const { return state_ == ConnectionState::CONNECTED; }
  void CheckPurchasedStatus();
  bool is_purchased_user() const { return is_purchased_user_; }

  void BindInterface(
      mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver);
  // mojom::vpn::ServiceHandler
  void GetConnectionState(GetConnectionStateCallback callback) override;
  void Connect() override;
  void Disconnect() override;
  void CreateVPNConnection() override;

 private:
  friend class BraveAppMenuBrowserTest;
  friend class BraveBrowserCommandControllerTest;

  // BraveVpnService overrides:
  void Shutdown() override;
  void AddObserver(
      mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> observer) override;

  // brave_vpn::BraveVPNOSConnectionAPI::Observer overrides:
  void OnCreated(const std::string& name) override;
  void OnRemoved(const std::string& name) override;
  void OnConnected(const std::string& name) override;
  void OnIsConnecting(const std::string& name) override;
  void OnConnectFailed(const std::string& name) override;
  void OnDisconnected(const std::string& name) override;
  void OnIsDisconnecting(const std::string& name) override;

  void set_is_purchased_user_for_test(bool purchased) {
    is_purchased_user_ = purchased;
  }

  brave_vpn::BraveVPNConnectionInfo GetConnectionInfo();

  ConnectionState state_ = ConnectionState::DISCONNECTED;
  bool is_purchased_user_ = false;
  base::ObserverList<Observer> observers_;
  base::ScopedObservation<brave_vpn::BraveVPNOSConnectionAPI,
                          brave_vpn::BraveVPNOSConnectionAPI::Observer>
      observed_{this};
  mojo::ReceiverSet<brave_vpn::mojom::ServiceHandler> receivers_;
  mojo::RemoteSet<brave_vpn::mojom::ServiceObserver> mojo_observers_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_DESKTOP_H_
