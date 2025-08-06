// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_component_updater/browser/component_contents_verifier.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/no_destructor.h"

namespace component_updater {

namespace {

ContentsVerifierFactory& GetContentsVerifierFactory() {
  static base::NoDestructor<ContentsVerifierFactory> factory;
  return *factory;
}

}  // namespace

void SetContentsVerifierFactory(ContentsVerifierFactory factory) {
  GetContentsVerifierFactory() = std::move(factory);
}

std::unique_ptr<ContentsVerifier> CreateContentsVerifier(
    const base::FilePath& component_root) {
  if (!GetContentsVerifierFactory()) {
    CHECK_IS_TEST();
    return nullptr;
  }
  return GetContentsVerifierFactory().Run(component_root);
}

}  // namespace component_updater
