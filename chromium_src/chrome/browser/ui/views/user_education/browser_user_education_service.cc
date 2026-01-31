/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/user_education/browser_user_education_service.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_education/common/feature_promo/feature_promo_registry.h"
#include "components/user_education/common/feature_promo/feature_promo_specification.h"

// This override allows Brave to register its own in-product help features with
// the user education system's feature promo registry.

namespace {

// Registers Brave-specific in-product help features.
void MaybeRegisterBraveFeaturePromos(
    user_education::FeaturePromoRegistry& registry,
    Profile* profile) {
  using user_education::FeaturePromoSpecification;
  using user_education::HelpBubbleArrow;

  // IPH for Brave Shields in Page Info.
  registry.RegisterFeature(std::move(
      FeaturePromoSpecification::CreateForToastPromo(
          feature_engagement::kIPHBraveShieldsInPageInfoFeature,
          kLocationIconElementId, IDS_BRAVE_SHIELDS_PAGE_INFO_IPH_TEXT,
          IDS_BRAVE_SHIELDS_PAGE_INFO_IPH_SCREENREADER_TEXT,
          FeaturePromoSpecification::AcceleratorInfo())
          .SetBubbleArrow(HelpBubbleArrow::kBottomCenter)
          .SetMetadata(144, "ksmith@brave.com",
                       "Shown when user visits a site for the first time after "
                       "the Shields UI was moved to Page Info. Educates users "
                       "about the new location.")));
}

}  // namespace

// BRAVE_MAYBE_REGISTER_CHROME_FEATURE_PROMOS is patched near the beginning of
// MaybeRegisterChromeFeaturePromos, and allows us to register Brave-specific
// in-product help features. Patching is necessary because the token
// "MaybeRegisterChromeFeaturePromos" appears several times in the source file
// in different contexts.
#define BRAVE_MAYBE_REGISTER_CHROME_FEATURE_PROMOS \
  MaybeRegisterBraveFeaturePromos(registry, profile);

#include <chrome/browser/ui/views/user_education/browser_user_education_service.cc>

#undef BRAVE_MAYBE_REGISTER_CHROME_FEATURE_PROMOS
