/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_UPDATER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_UPDATER_H_

#include <string>

namespace brave_shields {

class AdBlockRegionalUpdater {
 public:
  AdBlockRegionalUpdater(const std::string& component_name,
                         const std::string& component_id,
                         const std::string& component_base64_public_key);
  AdBlockRegionalUpdater(const AdBlockRegionalUpdater& other);
  ~AdBlockRegionalUpdater();

  const std::string component_name_;
  const std::string component_id_;
  const std::string component_base64_public_key_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_UPDATER_H_
