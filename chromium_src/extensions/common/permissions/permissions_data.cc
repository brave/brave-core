/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/extensions/common/brave_extension_urls.h"
#include "extensions/common/permissions/permissions_data.h"
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

  if (extension_urls::IsBraveProtectedUrl(url::Origin::Create(document_url),
                                          document_url.path_piece())) {
    if (error) {
      *error = kCannotScriptWalletLinking;
    }
    return true;
  }

  return false;
}

}  // namespace

#include "../../../../../extensions/common/permissions/permissions_data.cc"
