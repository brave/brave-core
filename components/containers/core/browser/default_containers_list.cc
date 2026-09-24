// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/default_containers_list.h"

#include "brave/ui/color/nala/nala_color_id.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_manager.h"

namespace containers {

std::vector<mojom::ContainerPtr> CreateDefaultContainersList() {
  const auto* color_provider =
      ui::ColorProviderManager::Get().GetColorProviderFor(
          ui::ColorProviderKey());
  CHECK(color_provider);

  std::vector<mojom::ContainerPtr> default_containers;

  default_containers.push_back(mojom::Container::New(
      kDefaultContainerIds[0],
      l10n_util::GetStringUTF8(IDS_CONTAINERS_DEFAULT_PERSONAL_NAME),
      mojom::Icon::kPersonal,
      color_provider->GetColor(nala::kColorPrimitiveBlue60)));
  default_containers.push_back(mojom::Container::New(
      kDefaultContainerIds[1],
      l10n_util::GetStringUTF8(IDS_CONTAINERS_DEFAULT_WORK_NAME),
      mojom::Icon::kWork,
      color_provider->GetColor(nala::kColorPrimitiveRed60)));
  default_containers.push_back(mojom::Container::New(
      kDefaultContainerIds[2],
      l10n_util::GetStringUTF8(IDS_CONTAINERS_DEFAULT_SOCIAL_NAME),
      mojom::Icon::kSocial,
      color_provider->GetColor(nala::kColorPrimitivePurple60)));
  default_containers.push_back(mojom::Container::New(
      kDefaultContainerIds[3],
      l10n_util::GetStringUTF8(IDS_CONTAINERS_DEFAULT_SCHOOL_NAME),
      mojom::Icon::kSchool,
      color_provider->GetColor(nala::kColorPrimitiveGreen60)));

  DCHECK_EQ(default_containers.size(), kDefaultContainerIds.size());

  return default_containers;
}

}  // namespace containers
