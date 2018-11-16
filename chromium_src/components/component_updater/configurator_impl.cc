/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/component_updater/component_updater_url_constants.h"

namespace component_updater {

const char kBraveUpdaterDefaultUrl[] =
    "https://componentupdater.brave.com/service/update2";

const char kBraveUpdaterFallbackUrl[] =
    "http://componentupdater.brave.com/service/update2";

}  // namespace component_updater

#define kUpdaterDefaultUrl kBraveUpdaterDefaultUrl
#define kUpdaterFallbackUrl kBraveUpdaterFallbackUrl
#include "../../../../components/component_updater/configurator_impl.cc"
#undef kUpdaterDefaultUrl
#undef kUpdaterFallbackUrl