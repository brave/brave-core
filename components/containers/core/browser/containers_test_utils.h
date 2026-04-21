// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_TEST_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_TEST_UTILS_H_

#include <string>

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "third_party/skia/include/core/SkColor.h"

namespace containers {

mojom::ContainerPtr MakeContainer(std::string id,
                                  std::string name,
                                  mojom::Icon icon = mojom::Icon::kDefault,
                                  SkColor color = SK_ColorBLUE);

void ExpectContainer(const mojom::ContainerPtr& container,
                     const std::string& id,
                     const std::string& name,
                     mojom::Icon icon = mojom::Icon::kDefault,
                     SkColor color = SK_ColorBLUE);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_TEST_UTILS_H_
