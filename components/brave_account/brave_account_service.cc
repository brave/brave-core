/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/brave_account/endpoints/verify/init.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service),
      url_loader_factory_(url_loader_factory),
      verify_init_(
          std::make_unique<endpoints::VerifyInit>(url_loader_factory_)) {
  pref_service_->SetString(prefs::kTest, "test");
  verify_init_->Send("sszaloki+aaa111@brave.com",
                     base::BindOnce([](endpoints::VerifyInit::Result result) {
                       VLOG(1) << result.response_code();
                       VLOG(1) << result.value_body();
                     }));
}

BraveAccountService::~BraveAccountService() = default;

}  // namespace brave_account
