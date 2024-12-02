/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/component_updater/pki_metadata_component_installer_unittest.cc"

namespace component_updater {

TEST_F(PKIMetadataComponentInstallerTest,
       InstallComponentUpdatesPinningIsDisabled) {
  // Initialize the network service.
  content::GetNetworkService();
  task_environment_.RunUntilIdle();
  WriteKPConfigToFile();
  policy_->ComponentReady(base::Version("1.2.3.4"),
                          component_install_dir_.GetPath(),
                          base::Value::Dict());
  task_environment_.RunUntilIdle();

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
}

}  // namespace component_updater
