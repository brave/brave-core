/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/component_updater/component_installer.h"

#include "components/crx_file/crx_verifier.h"
#define CRX3_WITH_PUBLISHER_PROOF CRX3
#define Register Register_ChromiumImpl
#include "../../../../components/component_updater/component_installer.cc"
#undef Register
#undef CRX3_WITH_PUBLISHER_PROOF

#include "base/stl_util.h"

namespace component_updater {

void ComponentInstaller::Register(ComponentUpdateService* cus,
                                  base::OnceClosure callback) {
  VLOG(1) << "Brave ComponentInstaller::Register override called.";
  static const char* disallowed_components[] = {
      "bklopemakmnopmghhmccadeonafabnal",  // Legacy TLS Deprecation Config
      "cmahhnpholdijhjokonmfdjbfmklppij",  // Federated Learning of Cohorts
      "eeigpngbgcognadeebkilcpcaedhellh",  // Autofill States Data
      "gcmjkmgdlgnkkcocmoeiminaijmmjnii",  // Subresource Filter Rules
      "ggkkehgbnfjpeggfpleeakpidbkibbmn",  // Crowd Deny
      "giekcmmlnklenlaomppkphknjmnnpneh",  // Certificate Error Assistant
      "jflookgnkcckhobaglndicnbbgbonegd",  // Safety Tips
      "llkgjffcdpffmhiakmfcdcblohccpfmo",  // Origin Trials
      "ojhpjlocmbogdgmfpkhlaaeamibhnphh",  // Zxcvbn Data Dictionaries
#if defined(OS_ANDROID)
      "lmelglejhemejginpboagddgdfbepgmp",  // Optimization Hints
      "obedbbhbpmojnkanicioggnmelmoomoc"   // OnDeviceHeadSuggest
#endif
  };
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(1) << "Brave ComponentInstaller::Register is on valid thread.";
  if (installer_policy_) {
    VLOG(1) << "Brave ComponentInstaller::Register has installer_policy_.";
    std::vector<uint8_t> hash;
    installer_policy_->GetHash(&hash);
    const std::string id = update_client::GetCrxIdFromPublicKeyHash(hash);
    VLOG(1) << "Brave ComponentInstaller::Register component id: " << id;
    if (base::Contains(disallowed_components, id.c_str())) {
      VLOG(1) << "Skipping registration of Brave-unsupported component "
              << id << ".";
      return;
    } else {
      VLOG(1) << "Brave ComponentInstaller::Register " << id
              << " is not blocked.";
    }
  } else {
    VLOG(1) << "Brave ComponentInstaller::Register has no installer_policy_.";
  }
  VLOG(1) << "Brave ComponentInstaller::Register calling base implementation.";
  Register_ChromiumImpl(cus, std::move(callback));
  VLOG(1) << "Brave ComponentInstaller::Register base implementation complete.";
}

}  // namespace component_updater
