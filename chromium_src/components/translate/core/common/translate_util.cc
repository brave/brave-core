/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "brave/components/translate/core/common/brave_translate_constants.h"

#define GetTranslateSecurityOrigin GetTranslateSecurityOrigin_Chromium
#include "../../../../../../components/translate/core/common/translate_util.cc"
#undef GetTranslateSecurityOrigin

namespace translate {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kTFLiteLanguageDetectionEnabled, base::FEATURE_DISABLED_BY_DEFAULT},
}});

// Redirect native translate requests to the translate.brave.com (expect the
// script request).
GURL GetTranslateSecurityOrigin() {
  std::string security_origin(kBraveTranslateOrigin);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kTranslateSecurityOrigin)) {
    security_origin =
        command_line->GetSwitchValueASCII(switches::kTranslateSecurityOrigin);
  }
  return GURL(security_origin);
}

}  // namespace translate
