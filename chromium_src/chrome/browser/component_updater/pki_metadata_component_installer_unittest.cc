/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/component_updater/pki_metadata_component_installer_unittest.cc>

namespace component_updater {

#if BUILDFLAG(CHROME_KEY_PINNING_SUPPORTED)
TEST_F(PKIMetadataComponentInstallerKeyPinningTest,
       InstallComponentUpdatesPinningIsDisabled) {
  // Initialize the network service and CertVerifierServiceFactory.
  content::GetNetworkService();
  content::GetCertVerifierServiceFactory();
  task_environment_.RunUntilIdle();
  WriteKPConfigToFile();
  // Brave ignores the update, but must still reply so observers are notified.
  PKIMetadataWaiter waiter;
  policy_->ComponentReady(base::Version("1.2.3.4"),
                          component_install_dir_.GetPath(), base::DictValue());
  waiter.Wait();

  network::NetworkService* network_service =
      network::NetworkService::GetNetworkServiceForTesting();
  ASSERT_TRUE(network_service);
  EXPECT_FALSE(network_service->pins_list_updated());
  const std::vector<net::TransportSecurityState::PinSet>& pinsets =
      network_service->pinsets();
  const std::vector<net::TransportSecurityState::PinSetInfo>& host_pins =
      network_service->host_pins();
  EXPECT_EQ(pinsets.size(), 0u);
  EXPECT_EQ(host_pins.size(), 0u);

  // Nor does the cert verifier, which would enforce them. Its compiled-in pins
  // are Brave's, so only the component's host is checked.
  cert_verifier::CertVerifierServiceFactoryImpl* cert_verifier_service_factory =
      content::GetCertVerifierServiceFactoryForTesting();
  ASSERT_TRUE(cert_verifier_service_factory);
  const net::CertVerifyProc::ImplParams& impl_params =
      cert_verifier_service_factory->get_impl_params();
  ASSERT_TRUE(impl_params.key_pinning_state);
  EXPECT_FALSE(
      impl_params.key_pinning_state->HasPublicKeyPins(kPinsetHostName));
}
#endif  // BUILDFLAG(CHROME_KEY_PINNING_SUPPORTED)

}  // namespace component_updater
