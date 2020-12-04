/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/component_updater/component_installer.h"

#include <algorithm>

#include "components/crx_file/crx_verifier.h"
#define CRX3_WITH_PUBLISHER_PROOF CRX3
#define Register Register_ChromiumImpl
#include "../../../../components/component_updater/component_installer.cc"
#undef Register
#undef CRX3_WITH_PUBLISHER_PROOF

namespace component_updater {

void ComponentInstaller::Register(ComponentUpdateService* cus,
                                  base::OnceClosure callback) {
  static std::vector<std::string> blacklisted_components({
      "bklopemakmnopmghhmccadeonafabnal", // Legacy TLS Deprecation Config
      "cmahhnpholdijhjokonmfdjbfmklppij", // Federated Learning of Cohorts
      "eeigpngbgcognadeebkilcpcaedhellh", // Autofill States Data
      "gcmjkmgdlgnkkcocmoeiminaijmmjnii", // Subresource Filter Rules
      "ggkkehgbnfjpeggfpleeakpidbkibbmn", // Crowd Deny
      "giekcmmlnklenlaomppkphknjmnnpneh", // Certificate Error Assistant
      "jflookgnkcckhobaglndicnbbgbonegd", // Safety Tips
      "llkgjffcdpffmhiakmfcdcblohccpfmo", // Origin Trials
      "ojhpjlocmbogdgmfpkhlaaeamibhnphh"  // Zxcvbn Data Dictionaries
  });
  std::vector<uint8_t> hash;
  installer_policy_->GetHash(&hash);
  std::string id = update_client::GetCrxIdFromPublicKeyHash(hash);

  if (std::find(blacklisted_components.begin(), blacklisted_components.end(),
                id) != blacklisted_components.end())
    VLOG(1) << "Skipping registration of Brave-unsupported component " << id
            << ".";
  else
    Register_ChromiumImpl(cus, std::move(callback));
}

}  // namespace component_updater
