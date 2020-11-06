/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/greaselion_api.h"

#include <memory>

#include "base/values.h"
#include "brave/browser/greaselion/greaselion_service_factory.h"
#include "brave/common/extensions/api/greaselion.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "chrome/browser/profiles/profile.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
GreaselionIsGreaselionExtensionFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  ::greaselion::GreaselionService* greaselion_service =
      ::greaselion::GreaselionServiceFactory::GetForBrowserContext(profile);
  if (!greaselion_service) {
    return RespondNow(OneArgument(base::Value(false)));
  }

  std::unique_ptr<greaselion::IsGreaselionExtension::Params> params(
      greaselion::IsGreaselionExtension::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  bool result = greaselion_service->IsGreaselionExtension(params->id);
  return RespondNow(OneArgument(base::Value(result)));
}

}  // namespace api
}  // namespace extensions
