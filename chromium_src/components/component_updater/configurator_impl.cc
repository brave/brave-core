/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/component_updater/configurator_impl.h"

#define EnabledBackgroundDownloader EnabledBackgroundDownloader_Unused
#define EnabledCupSigning EnabledCupSigning_Unused
#include "src/components/component_updater/configurator_impl.cc"
#undef EnabledCupSigning
#undef EnabledBackgroundDownloader

namespace component_updater {

bool ConfiguratorImpl::EnabledBackgroundDownloader() const {
  return false;
}

bool ConfiguratorImpl::EnabledCupSigning() const {
  return false;
}

}  // namespace component_updater
