/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVESYNC_BRAVESYNC_API_H
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVESYNC_BRAVESYNC_API_H

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class BraveSyncBackgroundPageToBrowserFunction : public UIThreadExtensionFunction {
  ~BraveSyncBackgroundPageToBrowserFunction() override {}
  DECLARE_EXTENSION_FUNCTION("braveSync.backgroundPageToBrowser", UNKNOWN)
  ResponseAction Run() override;
};

} // namespace api
} // namespace extensions

#endif // BRAVE_BROWSER_EXTENSIONS_API_BRAVESYNC_BRAVESYNC_API_H
