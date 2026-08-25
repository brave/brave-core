/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_UPDATE_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_UPDATE_CLIENT_H_

#define Uninstall(...)      \
  IsBraveComponent() const; \
  virtual bool Uninstall(__VA_ARGS__)

#include <components/update_client/update_client.h>  // IWYU pragma: export

#undef Uninstall

namespace update_client {

// Like `UpdateClientFactory`, but uses `SequentialUpdateChecker`.
scoped_refptr<UpdateClient> SequentialUpdateClientFactory(
    scoped_refptr<Configurator> config);

}  // namespace update_client

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_UPDATE_CLIENT_H_
