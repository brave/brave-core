/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/update_client/update_client.h"

#define UpdateClientFactory UpdateClientFactory_ChromiumImpl
#include "src/components/update_client/update_client.cc"
#undef UpdateClientFactory

#include "base/logging.h"

namespace update_client {

bool CrxInstaller::IsBraveComponent() const {
  return false;
}

scoped_refptr<UpdateClient> UpdateClientFactory(
    scoped_refptr<Configurator> config) {
  VLOG(3) << "Brave UpdateClientFactory called";
  return base::MakeRefCounted<UpdateClientImpl>(
      config, base::MakeRefCounted<PingManager>(config),
      base::BindRepeating(&SequentialUpdateChecker::Create));
}

}  // namespace update_client
