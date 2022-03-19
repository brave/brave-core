/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/component_updater/component_installer.h"

#define Register Register_ChromiumImpl
#include "src/components/component_updater/component_installer.cc"
#undef Register

#include "base/containers/contains.h"

namespace component_updater {

void ComponentInstaller::Register(ComponentUpdateService* cus,
                                  base::OnceClosure callback,
                                  base::TaskPriority task_priority) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(cus);
  Register(base::BindOnce(&ComponentUpdateService::RegisterComponent,
                          base::Unretained(cus)),
           std::move(callback), task_priority);
}

void ComponentInstaller::Register(RegisterCallback register_callback,
                                  base::OnceClosure callback,
                                  base::TaskPriority task_priority) {
  static std::string disallowed_components[] = {
    "bklopemakmnopmghhmccadeonafabnal",  // Legacy TLS Deprecation Config
    "cmahhnpholdijhjokonmfdjbfmklppij",  // Federated Learning of Cohorts
    "eeigpngbgcognadeebkilcpcaedhellh",  // Autofill States Data
    "gcmjkmgdlgnkkcocmoeiminaijmmjnii",  // Subresource Filter Rules
    "imefjhfbkmcmebodilednhmaccmincoa",  // Client Side Phishing Detection
    "llkgjffcdpffmhiakmfcdcblohccpfmo",  // Origin Trials
    "ojhpjlocmbogdgmfpkhlaaeamibhnphh",  // Zxcvbn Data Dictionaries
    "gonpemdgkjcecdgbnaabipppbmgfggbe",  // First Party Sets
    "dhlpobdgcjafebgbbhjdnapejmpkgiie",  // Desktop Sharing Hub
#if defined(OS_ANDROID)
    "lmelglejhemejginpboagddgdfbepgmp",  // Optimization Hints
    "obedbbhbpmojnkanicioggnmelmoomoc"   // OnDeviceHeadSuggest
#endif
  };
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (installer_policy_) {
    std::vector<uint8_t> hash;
    installer_policy_->GetHash(&hash);
    const std::string id = update_client::GetCrxIdFromPublicKeyHash(hash);
    if (base::Contains(disallowed_components, id)) {
      VLOG(1) << "Skipping registration of Brave-unsupported component "
              << id << ".";
      return;
    }
  }
  Register_ChromiumImpl(std::move(register_callback), std::move(callback),
                        task_priority);
}

}  // namespace component_updater
