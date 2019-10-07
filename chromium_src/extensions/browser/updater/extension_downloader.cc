/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/browser/updater/extension_downloader.h"

#include "base/strings/string_piece.h"
#include "brave/browser/net/brave_simple_url_loader.h"

#define SimpleURLLoader BraveSimpleURLLoader
#include "../../../../../extensions/browser/updater/extension_downloader.cc"  // NOLINT
#undef SimpleURLLoader
