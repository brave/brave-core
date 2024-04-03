// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/vector_icons/vector_icons.h"
#include "components/omnibox/browser/vector_icons.h"

namespace {
constexpr char kLeoWindowTabNewIconResourceName[] =
    "//resources/brave-icons/window-tab-new.svg";
}  // namespace

// This patches |ActionVectorIconToResourceName| to include the additional
// omnibox icons we add in Brave. Unfortunately, the method is used inside the
// file, so we can't just override the method. There is only one usage of
// kSwitchIcon in the file, so we add additional statements from it.
#define kSwitchIcon kSwitchIcon.name) {}        \
  if (icon.name == kLeoWindowTabNewIcon.name) { \
    return kLeoWindowTabNewIconResourceName;    \
  }                                             \
  if (icon.name == omnibox::kSwitchIcon

#include "src/chrome/browser/ui/webui/searchbox/searchbox_handler.cc"

#undef kSwitchIcon
