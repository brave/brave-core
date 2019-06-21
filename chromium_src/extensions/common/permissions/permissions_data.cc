/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/common/permissions/permissions_data.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

const char kCannotScriptWalletLinking[] =
    "Pages part of the wallet linking flow cannot be scripted without user "
    "interaction.";

bool IsBraveRestrictedUrl(const GURL& document_url,
    const extensions::ExtensionId& extension_id,
    extensions::Manifest::Location location,
    std::string* error) {

  if (extensions::PermissionsData::CanExecuteScriptEverywhere(extension_id,
                                                              location)) {
    return false;
  }

  url::Origin origin = url::Origin::Create(document_url);
  if ((origin.DomainIs("sandbox.uphold.com") ||
       origin.DomainIs("uphold.com")) &&
      base::StartsWith(document_url.path_piece(), "/authorize/",
                       base::CompareCase::SENSITIVE)) {
    if (error) {
      *error = kCannotScriptWalletLinking;
    }
    return true;
  }

  return false;
}

}  // namespace

#include "../../../../../extensions/common/permissions/permissions_data.cc"
