/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/grit/component_extension_resources.h"
#include "brave/browser/resources/extensions/grit/brave_extensions_resources.h"

#undef IDR_PDF_VIEWPORT_JS
#define IDR_PDF_VIEWPORT_JS  IDR_BRAVE_PDF_VIEWPORT_JS
#include "../../../../../../../chrome/browser/ui/webui/print_preview/print_preview_ui.cc"  // NOLINT
#undef IDR_PDF_VIEWPORT_JS
