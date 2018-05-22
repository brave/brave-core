/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_regional_updater.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace brave_shields {

AdBlockRegionalUpdater::AdBlockRegionalUpdater(
    const std::string& component_name,
    const std::string& component_id,
    const std::string& component_base64_public_key)
    : component_name_(component_name),
      component_id_(component_id),
      component_base64_public_key_(component_base64_public_key) {
}

AdBlockRegionalUpdater::AdBlockRegionalUpdater(const AdBlockRegionalUpdater& other) = default;

AdBlockRegionalUpdater::~AdBlockRegionalUpdater() {
}

}  // namespace brave_shields
