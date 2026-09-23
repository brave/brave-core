/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/update_client/update_client.h"

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "components/update_client/update_checker.h"

#include <components/update_client/update_client.cc>

namespace update_client {

bool CrxInstaller::IsBraveComponent() const {
  return false;
}

scoped_refptr<UpdateClient> SequentialUpdateClientFactory(
    scoped_refptr<Configurator> config) {
  return base::MakeRefCounted<UpdateClientImpl>(
      config, base::MakeRefCounted<PingManager>(config),
      base::BindRepeating(&SequentialUpdateChecker::Create));
}

}  // namespace update_client
