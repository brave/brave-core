/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_safari_importer.h"

#define SafariImporter BraveSafariImporter
#include "../../../../../chrome/utility/importer/importer_creator.cc"
#undef SafariImporter
