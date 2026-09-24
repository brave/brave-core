// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_COMMAND_LINE_CONTAINER_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_COMMAND_LINE_CONTAINER_H_

#include "brave/components/containers/core/browser/container_specifier.h"

namespace base {
class CommandLine;
}  // namespace base

namespace containers {

class ContainersService;

// Returns the container to use for the tabs passed via the command line. All
// command line tabs share the same container, matching the "open in new
// temporary container" UI.
//
// `--temporary-container` opens the tabs in a temporary container: a new one,
// or, when `--container` also gives a name, the temporary container with that
// name, created on first use so that a later launch can open more tabs in it.
// `--container` on its own resolves an existing container by name.
//
// Creating a temporary container persists it, so this must not be called more
// than once per launch. Returns an empty specifier (no container) when neither
// switch is given.
ContainerSpecifier GetContainerSpecifierForCommandLineTabs(
    const base::CommandLine& command_line,
    ContainersService* containers_service);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_COMMAND_LINE_CONTAINER_H_
