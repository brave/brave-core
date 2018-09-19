/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_network_delegate.h"

#define ChromeNetworkDelegate BraveSystemNetworkDelegate
#include "../../../../chrome/browser/io_thread.cc"
#undef ChromeNetworkDelegate
