/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_CONFIGURATOR_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_CONFIGURATOR_IMPL_H_

// Prevent EnabledBackgroundDownloader from being redefined in the interface.
#include "components/update_client/configurator.h"

#define EnabledBackgroundDownloader                 \
  EnabledBackgroundDownloader_ChromiumImpl() const; \
  bool EnabledBackgroundDownloader

#include "src/components/component_updater/configurator_impl.h"

#undef EnabledBackgroundDownloader

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_COMPONENT_UPDATER_CONFIGURATOR_IMPL_H_
