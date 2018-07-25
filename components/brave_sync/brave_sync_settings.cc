/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_settings.h"

namespace brave_sync {

BraveSyncSettings::BraveSyncSettings() :
  sync_this_device_(false),
  sync_bookmarks_(false),
  sync_settings_(false),
  sync_history_(false),
  sync_configured_(false) {
}

} // namespace brave_sync
