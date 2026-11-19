/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/run_until.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/profile_network_context_service_factory.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/fido/cable/pairing.h"
#include "device/fido/cable/v2_constants.h"
#include "device/fido/cable/v2_handshake.h"
#include "device/fido/fido_discovery_base.h"
#include "device/fido/fido_discovery_factory.h"
#include "device/fido/public/fido_constants.h"
#include "device/fido/public/fido_transport_protocol.h"
#include "device/fido/public/fido_types.h"
#include "net/base/net_errors.h"
#include "net/test/embedded_test_server/controllable_http_response.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(IS_MAC)
#include "base/process/process_info.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/tor/tor_profile_service_impl.h"
#endif

namespace {

constexpr char kTunnelServerErrorHistogram[] =
    "WebAuthentication.CableV2.TunnelServerError";

// The authority a proxy sees in the CONNECT request for a caBLE tunnel.
std::string TunnelServerAuthority() {
  return base::StrCat({device::cablev2::tunnelserver::DecodeDomain(
                           device::cablev2::kTunnelServer),
                       ":443"});
}

// Counts the CONNECT requests a proxy receives for the caBLE tunnel server.
class TunnelRequestCounter {
 public:
  void Monitor(net::EmbeddedTestServer& server) {
    server.RegisterRequestMonitor(base::BindRepeating(
        &TunnelRequestCounter::OnRequest, base::Unretained(this)));
  }

  int count() const { return count_; }

 private:
  void OnRequest(const net::test_server::HttpRequest& request) {
    if (request.method == net::test_server::METHOD_CONNECT &&
        request.relative_url == TunnelServerAuthority()) {
      ++count_;
    }
  }

  std::atomic<int> count_ = 0;
};

}  // namespace

// A WebAuthn request's caBLE tunnel to the user's phone must go through the
// requesting frame's own network context in an off-the-record profile, never
// the browser-wide system network context, and through Tor in a Tor window.
// Each context gets a proxy of its own, so the proxy that receives the tunnel's
// CONNECT request shows which context opened it.
class OffTheRecordCableBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
#if BUILDFLAG(IS_MAC)
    // Hybrid discovery makes no Bluetooth calls, and so opens no tunnel, when
    // the app responsible for the test process lacks Bluetooth metadata.
    if (!base::DoesResponsibleProcessHaveBluetoothMetadata()) {
      GTEST_SKIP() << "The responsible process has no Bluetooth metadata.";
    }
#endif
    bluetooth_values_ =
        device::BluetoothAdapterFactory::Get()->InitGlobalOverrideValues();
    bluetooth_values_->SetLESupported(true);

    // Must exist before the servers start. They hold a tunnel's CONNECT
    // request at the proxy it reaches, so the tunnel stays pending.
    held_system_response_ =
        std::make_unique<net::test_server::ControllableHttpResponse>(
            &system_proxy_, TunnelServerAuthority());
    held_profile_response_ =
        std::make_unique<net::test_server::ControllableHttpResponse>(
            &profile_proxy_, TunnelServerAuthority());
    system_counter_.Monitor(system_proxy_);
    profile_counter_.Monitor(profile_proxy_);
    ASSERT_TRUE(system_proxy_.Start());
    ASSERT_TRUE(profile_proxy_.Start());

    g_browser_process->local_state()->SetDict(
        proxy_config::prefs::kProxy,
        ProxyConfigDictionary::CreateFixedServers(
            system_proxy_.host_port_pair().ToString(), std::string()));
    g_browser_process->system_network_context_manager()
        ->FlushProxyConfigMonitorForTesting();
  }

  // Points `profile`'s network context at `profile_proxy_`.
  void UseProfileProxy(Profile* profile) {
    profile->GetPrefs()->SetDict(
        proxy_config::prefs::kProxy,
        ProxyConfigDictionary::CreateFixedServers(
            profile_proxy_.host_port_pair().ToString(), std::string()));
    ProfileNetworkContextServiceFactory::GetForContext(profile)
        ->FlushProxyConfigMonitorForTesting();
  }

  // Runs a WebAuthn request's discovery setup for the active tab of
  // `browser_window`, then has a paired phone's tunnel opened the way caBLE
  // contacts a linked device. The tunnel stays open while `delegate_` and
  // `discoveries_` are alive.
  void OpenPairedPhoneTunnel(BrowserWindowInterface* browser_window) {
    content::RenderFrameHost* frame = browser_window->GetTabStripModel()
                                          ->GetActiveWebContents()
                                          ->GetPrimaryMainFrame();
    delegate_ = std::make_unique<ChromeAuthenticatorRequestDelegate>(frame);
    delegate_->SetRelyingPartyId("example.com");
    delegate_->ConfigureDiscoveries(
        url::Origin::Create(GURL("https://example.com")), "example.com",
        content::AuthenticatorRequestClientDelegate::RequestSource::
            kWebAuthentication,
        device::FidoRequestType::kGetAssertion,
        /*resident_key_requirement=*/std::nullopt,
        device::UserVerificationRequirement::kRequired,
        /*cmtg_key_requested=*/false,
        /*user_name=*/std::nullopt,
        /*browser_provided_passkeys_available=*/false, &discovery_factory_);
#if BUILDFLAG(IS_WIN)
    discovery_factory_.set_force_hybrid_discovery(true);
#endif
    auto contact_phone = discovery_factory_.get_cable_contact_callback();
    discoveries_ =
        discovery_factory_.Create(device::FidoTransportProtocol::kHybrid);
    ASSERT_EQ(1u, discoveries_.size());

    auto pairing = std::make_unique<device::cablev2::Pairing>();
    pairing->contact_id = {1, 2, 3, 4};
    pairing->id = {5, 6, 7, 8};
    pairing->secret = std::vector<uint8_t>(32, 9);
    contact_phone.Run(std::move(pairing));
  }

  // Waits until a proxy receives the tunnel's CONNECT request.
  [[nodiscard]] bool WaitForTunnelRequest() {
    return base::test::RunUntil(
        [&]() { return system_requests() + profile_requests() > 0; });
  }

  void TearDownOnMainThread() override {
    discoveries_.clear();
    delegate_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  int system_requests() const { return system_counter_.count(); }
  int profile_requests() const { return profile_counter_.count(); }

 private:
  std::unique_ptr<device::BluetoothAdapterFactory::GlobalOverrideValues>
      bluetooth_values_;
  TunnelRequestCounter system_counter_;
  TunnelRequestCounter profile_counter_;
  // Declared after the counters their monitors use, so the servers shut down
  // before the counters are destroyed.
  net::EmbeddedTestServer system_proxy_;
  net::EmbeddedTestServer profile_proxy_;
  std::unique_ptr<net::test_server::ControllableHttpResponse>
      held_system_response_;
  std::unique_ptr<net::test_server::ControllableHttpResponse>
      held_profile_response_;
  device::FidoDiscoveryFactory discovery_factory_;
  std::unique_ptr<ChromeAuthenticatorRequestDelegate> delegate_;
  std::vector<std::unique_ptr<device::FidoDiscoveryBase>> discoveries_;
};

IN_PROC_BROWSER_TEST_F(OffTheRecordCableBrowserTest,
                       PrivateWindowTunnelUsesProfileNetworkContext) {
  BrowserWindowInterface* private_browser = CreateIncognitoBrowser();
  UseProfileProxy(private_browser->GetProfile());

  OpenPairedPhoneTunnel(private_browser);
  ASSERT_TRUE(WaitForTunnelRequest());

  EXPECT_EQ(1, profile_requests())
      << "The tunnel did not go through the Private window's network context.";
  EXPECT_EQ(0, system_requests())
      << "The tunnel went through the system network context.";
}

IN_PROC_BROWSER_TEST_F(OffTheRecordCableBrowserTest,
                       RegularWindowTunnelIsUnchanged) {
  OpenPairedPhoneTunnel(browser());
  ASSERT_TRUE(WaitForTunnelRequest());

  EXPECT_EQ(1, system_requests())
      << "A regular window's tunnel no longer uses the system network context.";
  EXPECT_EQ(0, profile_requests());
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(OffTheRecordCableBrowserTest,
                       TorWindowTunnelGoesThroughTor) {
  BrowserWindowInterface* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->GetProfile());
  Profile* tor_profile = tor_browser->GetProfile();
  ASSERT_TRUE(tor_profile->IsTor());

  // Points Tor at a SOCKS proxy that refuses connections, so a tunnel sent
  // through Tor fails with ERR_PROXY_CONNECTION_FAILED.
  net::EmbeddedTestServer closed_server;
  ASSERT_TRUE(closed_server.Start());
  const std::string closed_host_port =
      closed_server.host_port_pair().ToString();
  ASSERT_TRUE(closed_server.ShutdownAndWaitUntilComplete());
  static_cast<tor::TorProfileServiceImpl*>(
      TorProfileServiceFactory::GetForContext(tor_profile))
      ->OnTorNewProxyURI(base::StrCat({"socks5://", closed_host_port}));
  ProfileNetworkContextServiceFactory::GetForContext(tor_profile)
      ->FlushProxyConfigMonitorForTesting();

  base::HistogramTester histogram_tester;
  OpenPairedPhoneTunnel(tor_browser);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return system_requests() > 0 || histogram_tester.GetBucketCount(
                                        kTunnelServerErrorHistogram,
                                        net::ERR_PROXY_CONNECTION_FAILED) > 0;
  }));

  EXPECT_EQ(0, system_requests())
      << "The tunnel went through the system network context.";
  histogram_tester.ExpectUniqueSample(kTunnelServerErrorHistogram,
                                      net::ERR_PROXY_CONNECTION_FAILED, 1);
}
#endif  // BUILDFLAG(ENABLE_TOR)
