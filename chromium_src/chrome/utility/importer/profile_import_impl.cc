/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_external_process_importer_bridge.h"

#define ExternalProcessImporterBridge BraveExternalProcessImporterBridge
#include "../../../../../chrome/utility/importer/profile_import_impl.cc"
#undef ExternalProcessImporterBridge
