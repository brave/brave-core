/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_piece.h"
#include "extensions/common/permissions/permissions_data.h"
#include "url/origin.h"

namespace extensions {

bool IsBraveProtectedUrl(const GURL& url) {
  const url::Origin origin = url::Origin::Create(url);
  const base::StringPiece path = url.path_piece();
  return ((origin.DomainIs("sandbox.uphold.com") ||
           origin.DomainIs("uphold.com")) &&
          base::StartsWith(path, "/authorize/",
                           base::CompareCase::INSENSITIVE_ASCII)) ||
      (origin.DomainIs("api.uphold.com") &&
       base::StartsWith(path, "/oauth2/token",
                        base::CompareCase::INSENSITIVE_ASCII));
}

}  // namespace extensions

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

  if (extensions::IsBraveProtectedUrl(document_url)) {
    if (error) {
      *error = kCannotScriptWalletLinking;
    }
    return true;
  }

  return false;
}

}  // namespace

// Disable some content scripts until users click on the extension icon
#define BRAVE_CAN_RUN_ON_PAGE \
if (IsBraveRestrictedUrl(document_url, extension_id_, location_, error)) { \
  return PageAccess::kWithheld;\
}

#include "../../../../../extensions/common/permissions/permissions_data.cc"
