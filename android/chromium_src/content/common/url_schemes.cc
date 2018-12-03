/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/url_constants.h"
#define RegisterContentSchemes RegisterContentSchemes_ChromiumImpl
#include "../../../../../content/common/url_schemes.cc"
#undef RegisterContentSchemes

namespace {

void RegisterContentSchemes_BraveImpl(bool lock_schemes) {
  url::AddStandardScheme(kBraveUIScheme, url::SCHEME_WITH_HOST);
}

}

namespace content {

void RegisterContentSchemes(bool lock_schemes) {
  RegisterContentSchemes_BraveImpl(lock_schemes);
  RegisterContentSchemes_ChromiumImpl(lock_schemes);
}

}  // namespace content
