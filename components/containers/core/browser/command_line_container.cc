// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/command_line_container.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/common/switches.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

ContainerSpecifier GetContainerSpecifierForCommandLineTabs(
    const base::CommandLine& command_line,
    ContainersService* containers_service) {
  CHECK(containers_service);

  const std::string container_name =
      command_line.GetSwitchValueUTF8(switches::kContainer);

  if (command_line.HasSwitch(switches::kTemporaryContainer)) {
    auto container =
        container_name.empty()
            ? containers_service->CreateAndPersistTemporaryContainer()
            : containers_service->GetOrCreateTemporaryContainerByName(
                  container_name);
    // Address the temporary container by id: names are not guaranteed to be
    // unique, and a generated one is random.
    return ContainerId(std::move(container->id));
  }

  if (!container_name.empty()) {
    return ContainerName(container_name);
  }

  return {};
}

}  // namespace containers
