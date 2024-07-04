/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_provider.h"

#include <string>

#include "extensions/common/constants.h"

namespace extensions {

BraveExtensionProvider::BraveExtensionProvider() = default;

BraveExtensionProvider::~BraveExtensionProvider() = default;

std::string BraveExtensionProvider::GetDebugPolicyProviderName() const {
#if defined(NDEBUG)
  NOTREACHED_IN_MIGRATION();
  return std::string();
#else
  return "Brave Extension Provider";
#endif
}

bool BraveExtensionProvider::MustRemainInstalled(const Extension* extension,
                                                 std::u16string* error) const {
  return extension->id() == brave_extension_id;
}

}  // namespace extensions
