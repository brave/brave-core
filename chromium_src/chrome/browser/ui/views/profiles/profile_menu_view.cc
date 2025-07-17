/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/profiles/profile_menu_view_base.h"
#include "ui/gfx/vector_icon_types.h"

namespace {

ProfileMenuViewBase::IdentitySectionParams BraveAdjustIdentitySectionParams(
    ProfileMenuViewBase::IdentitySectionParams params) {
  // We never show header, subtitle, or action button.
  ProfileMenuViewBase::IdentitySectionParams new_params;
  new_params.profile_image = params.profile_image;
  new_params.profile_image_padding = params.profile_image_padding;
  new_params.title = params.title;
  new_params.has_dotted_ring = params.has_dotted_ring;
  return new_params;
}

}  // namespace

#define SetProfileIdentityWithCallToAction(PARAMS) \
  SetProfileIdentityWithCallToAction(BraveAdjustIdentitySectionParams(PARAMS))

#include "src/chrome/browser/ui/views/profiles/profile_menu_view.cc"
#undef SetProfileIdentityWithCallToAction
