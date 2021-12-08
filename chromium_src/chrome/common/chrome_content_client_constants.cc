/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/chrome_content_client.h"

#define kPDFExtensionPluginName kPDFExtensionPluginName_Unused
#define kPDFInternalPluginName kPDFInternalPluginName_Unused

#include "src/chrome/common/chrome_content_client_constants.cc"

#undef kPDFExtensionPluginName
#undef kPDFInternalPluginName

const char ChromeContentClient::kPDFExtensionPluginName[] = "Chrome PDF Viewer";
const char ChromeContentClient::kPDFInternalPluginName[] = "Chrome PDF Plugin";
