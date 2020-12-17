/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_IDENTITY_MANAGER_BUILDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_IDENTITY_MANAGER_BUILDER_H_

#include <memory>

#include "components/signin/public/identity_manager/identity_manager.h"

namespace signin {

class BraveIdentityManager;
struct IdentityManagerBuildParams;

// Builds an IdentityManager instance from the supplied embedder-level
// dependencies.
std::unique_ptr<BraveIdentityManager> BuildBraveIdentityManager(
    IdentityManagerBuildParams* params);

}  // namespace signin

#include "../../../../../../components/signin/public/identity_manager/identity_manager_builder.h"

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_IDENTITY_MANAGER_BUILDER_H_
