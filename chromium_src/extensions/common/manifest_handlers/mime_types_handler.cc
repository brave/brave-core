/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../extensions/common/manifest_handlers/mime_types_handler.cc"

// static
std::vector<std::string> MimeTypesHandler::BraveGetMIMETypeWhitelist() {
  std::vector<std::string> whitelist = MimeTypesHandler::GetMIMETypeWhitelist();
  auto pos = std::find(whitelist.begin(), whitelist.end(),
                       extension_misc::kPdfExtensionId);
  if (pos != whitelist.end())
    whitelist.erase(pos);
  return whitelist;
}

