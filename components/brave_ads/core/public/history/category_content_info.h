/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_CATEGORY_CONTENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_CATEGORY_CONTENT_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/export.h"

namespace brave_ads {

struct ADS_EXPORT CategoryContentInfo final {
  bool operator==(const CategoryContentInfo&) const = default;

  std::string category;
  mojom::UserReactionType user_reaction_type =
      mojom::UserReactionType::kNeutral;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_HISTORY_CATEGORY_CONTENT_INFO_H_
