/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/power_monitor/power_observer.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_vpn/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/mojom/brave_vpn.mojom.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

class PrefService;

namespace brave_vpn {

struct Hostname;

// Interface for managing OS' vpn connection.
class BraveVPNOSConnectionAPI : public base::PowerSuspendObserver,
                                public net::NetworkChangeNotifier::DNSObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnGetInvalidToken() = 0;
    virtual void OnConnectionStateChanged(mojom::ConnectionState state) = 0;

   protected:
    ~Observer() override = default;
  };

  static BraveVPNOSConnectionAPI* GetInstance();
  static BraveVPNOSConnectionAPI* GetInstanceForTest();

  BraveVPNOSConnectionAPI(const BraveVPNOSConnectionAPI&) = delete;
  BraveVPNOSConnectionAPI& operator=(const BraveVPNOSConnectionAPI&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void set_shared_url_loader_factory(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
    url_loader_factory_ = url_loader_factory;
  }

  void set_local_prefs(PrefService* prefs) { local_prefs_ = prefs; }
  void set_target_vpn_entry_name(const std::string& name) {
    target_vpn_entry_name_ = name;
  }

  const BraveVPNConnectionInfo& connection_info() const {
    return connection_info_;
  }
  mojom::ConnectionState connection_state() const { return connection_state_; }

  void SetConnectionState(mojom::ConnectionState state);
  bool IsInProgress() const;

  void Connect();
  void Disconnect();
  void ToggleConnection();
  void RemoveVPNConnection();
  void CheckConnection();
  void SetSkusCredential(const std::string& credential);
  void ResetConnectionInfo();
  std::string GetHostname() const;

 protected:
  BraveVPNOSConnectionAPI();
  ~BraveVPNOSConnectionAPI() override;

  // Subclass should add platform dependent impls.
  virtual void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) = 0;
  virtual void ConnectImpl(const std::string& name) = 0;
  virtual void DisconnectImpl(const std::string& name) = 0;
  virtual void RemoveVPNConnectionImpl(const std::string& name) = 0;
  virtual void CheckConnectionImpl(const std::string& name) = 0;
  virtual bool GetIsSimulation() const;

  // Subclass should call below callbacks whenever corresponding event happens.
  void OnCreated();
  void OnCreateFailed();
  void OnConnected();
  void OnIsConnecting();
  void OnConnectFailed();
  void OnDisconnected();
  void OnIsDisconnecting();

  std::string target_vpn_entry_name() const { return target_vpn_entry_name_; }

 private:
  friend class BraveVPNServiceTest;

  using URLRequestCallback =
      base::OnceCallback<void(api_request_helper::APIRequestResult)>;
  using ResponseCallback =
      base::OnceCallback<void(const std::string&, bool success)>;

  // base::PowerMonitor
  void OnSuspend() override;
  void OnResume() override;

  // net::NetworkChangeNotifier::DNSObserver
  void OnDNSChanged() override;

  void CreateVPNConnection();
  std::string GetSelectedRegion() const;
  std::string GetDeviceRegion() const;
  std::string GetCurrentEnvironment() const;
  void FetchHostnamesForRegion(const std::string& name);
  void GetHostnamesForRegion(ResponseCallback callback,
                             const std::string& region);
  void OnFetchHostnames(const std::string& region,
                        const std::string& hostnames,
                        bool success);
  void ParseAndCacheHostnames(const std::string& region,
                              const base::Value::List& hostnames_value);
  void GetSubscriberCredentialV12(ResponseCallback callback);
  void OnGetSubscriberCredentialV12(const std::string& subscriber_credential,
                                    bool success);
  void OnGetSubscriberCredential(
      ResponseCallback callback,
      api_request_helper::APIRequestResult api_request_result);
  void GetProfileCredentials(ResponseCallback callback,
                             const std::string& subscriber_credential,
                             const std::string& hostname);
  void OnGetProfileCredentials(const std::string& profile_credential,
                               bool success);

  void UpdateAndNotifyConnectionStateChange(mojom::ConnectionState state);
  api_request_helper::APIRequestHelper* GetAPIRequestHelper();

  void OAuthRequest(
      const GURL& url,
      const std::string& method,
      const std::string& post_data,
      URLRequestCallback callback,
      const base::flat_map<std::string, std::string>& headers = {});
  void OnGetResponse(ResponseCallback callback,
                     api_request_helper::APIRequestResult result);

  bool cancel_connecting_ = false;
  bool needs_connect_ = false;
  bool reconnect_on_resume_ = false;
  bool prevent_creation_ = false;
  std::string target_vpn_entry_name_;
  std::string skus_credential_;
  mojom::ConnectionState connection_state_ =
      mojom::ConnectionState::DISCONNECTED;
  BraveVPNConnectionInfo connection_info_;
  raw_ptr<PrefService> local_prefs_ = nullptr;
  std::unique_ptr<Hostname> hostname_;
  base::ObserverList<Observer> observers_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<BraveVPNOSConnectionAPI> weak_ptr_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_H_
