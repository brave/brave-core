/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_BRAVE_IDENTITY_MANAGER_BUILDER_H_
#define BRAVE_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_BRAVE_IDENTITY_MANAGER_BUILDER_H_

#include <memory>

#include "components/signin/public/identity_manager/identity_manager.h"

// To avoid the code duplication, implementation is done in
// chromium_src/components/signin/public/identity_manager/identity_manager_builder.cc

namespace signin {

enum class AccountConsistencyMethod;
class BraveIdentityManager;
struct IdentityManagerBuildParams;

// Builds all required dependencies to initialize the IdentityManager instance.
IdentityManager::InitParameters BuildBraveIdentityManagerInitParameters(
    IdentityManagerBuildParams* params);

// Builds an IdentityManager instance from the supplied embedder-level
// dependencies.
std::unique_ptr<BraveIdentityManager> BuildBraveIdentityManager(
    IdentityManagerBuildParams* params);

}  // namespace signin

#endif  // BRAVE_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_BRAVE_IDENTITY_MANAGER_BUILDER_H_
