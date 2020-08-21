/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_EXPERIMENTS_EXPERIMENTS_USER_MODELS_H_  // NOLINT
#define BAT_ADS_INTERNAL_EXPERIMENTS_EXPERIMENTS_USER_MODELS_H_  // NOLINT

#include <map>
#include <set>
#include <string>

namespace ads {

const std::map<std::string, std::string> kExperimentsCountryCodes = {
  {
    "US", "oaclohknclldeiojdglnllhknnobmibf"
  },
  {
    "GB", "foijmhkdleilhmodmodoeddoenmpblcp"
  },
  {
    "FR", "jnbmapekcbejhichjihpobmjgjemmbpb"
  },
  {
    "IN", "ngggnncjiioglpfllbgbnenmfhlebkoe"
  },
  {
    "DE", "flbeddbpalboefcmfedaoghccejppcbc"
  }
};

const std::set<std::string> kExperimentsUserModelIds = {
  "oaclohknclldeiojdglnllhknnobmibf",
  "foijmhkdleilhmodmodoeddoenmpblcp",
  "jnbmapekcbejhichjihpobmjgjemmbpb",
  "ngggnncjiioglpfllbgbnenmfhlebkoe",
  "flbeddbpalboefcmfedaoghccejppcbc"
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_EXPERIMENTS_EXPERIMENTS_USER_MODELS_H_  // NOLINT
