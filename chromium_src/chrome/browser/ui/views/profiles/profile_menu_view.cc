/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/app/vector_icons/vector_icons.h"
#include "ui/gfx/vector_icon_types.h"

namespace {
constexpr gfx::VectorIcon kGuestMenuEmptyArtIcon;
}  // namespace

#define kGuestMenuArtIcon kGuestMenuEmptyArtIcon
#include "../../../../../../../chrome/browser/ui/views/profiles/profile_menu_view.cc"
#undef kGuestMenuArtIcon
