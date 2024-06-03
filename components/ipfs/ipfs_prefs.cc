/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_prefs.h"
#include "base/files/file_path.h"
#include "components/prefs/pref_registry_simple.h"

namespace ipfs {

void RegisterDeprecatedIpfsPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kIPFSEnabled, true);
  registry->RegisterIntegerPref(kIPFSResolveMethod,0);
  registry->RegisterBooleanPref(kIPFSAutoFallbackToGateway, false);
  registry->RegisterBooleanPref(kIPFSAlwaysStartMode, false);

  registry->RegisterBooleanPref(kIPFSAutoRedirectToConfiguredGateway, false);
  registry->RegisterBooleanPref(kIPFSLocalNodeUsed, false);
  registry->RegisterIntegerPref(kIPFSInfobarCount, 0);
  registry->RegisterIntegerPref(kIpfsStorageMax, 1);
  registry->RegisterStringPref(kIPFSPublicGatewayAddress, "");
  registry->RegisterStringPref(kIPFSPublicNFTGatewayAddress,"");
  registry->RegisterFilePathPref(kIPFSBinaryPath, base::FilePath());
  registry->RegisterDictionaryPref(kIPFSPinnedCids);
  registry->RegisterBooleanPref(kShowIPFSPromoInfobar, true);
  registry->RegisterBooleanPref(kIPFSAlwaysStartInfobarShown, false);

  // Deprecated, kIPFSAutoRedirectToConfiguredGateway is used instead
  registry->RegisterBooleanPref(kIPFSAutoRedirectGateway, false);
  registry->RegisterBooleanPref(kIPFSAutoRedirectDNSLink, false);

  registry->RegisterBooleanPref(kIPFSCompanionEnabled, false);
}

void ClearDeprecatedIpfsPrefs(PrefService* registry) {
  registry->ClearPref(kIPFSEnabled);
  registry->ClearPref(kIPFSResolveMethod);
  registry->ClearPref(kIPFSAutoFallbackToGateway);
  registry->ClearPref(kIPFSAlwaysStartMode);

  registry->ClearPref(kIPFSAutoRedirectToConfiguredGateway);
  registry->ClearPref(kIPFSLocalNodeUsed);
  registry->ClearPref(kIPFSInfobarCount);
  registry->ClearPref(kIpfsStorageMax);
  registry->ClearPref(kIPFSPublicGatewayAddress);
  registry->ClearPref(kIPFSPublicNFTGatewayAddress);
  registry->ClearPref(kIPFSBinaryPath);
  registry->ClearPref(kIPFSPinnedCids);
  registry->ClearPref(kShowIPFSPromoInfobar);
  registry->ClearPref(kIPFSAlwaysStartInfobarShown);
  registry->ClearPref(kIPFSAutoRedirectGateway);
  registry->ClearPref(kIPFSAutoRedirectDNSLink);
  registry->ClearPref(kIPFSCompanionEnabled);
}

}  // namespace ipfs
