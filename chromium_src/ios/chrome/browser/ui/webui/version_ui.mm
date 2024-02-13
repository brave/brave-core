// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/version_info/version_info.h"
#include "components/version_info/version_info.h"

#define GetVersionNumber GetBraveVersionNumberForDisplay
#include "src/ios/chrome/browser/ui/webui/version_ui.mm"
#undef GetVersionNumber
