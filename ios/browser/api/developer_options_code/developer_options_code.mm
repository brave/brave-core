// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/developer_options_code/developer_options_code.h"

#include "brave/ios/browser/api/developer_options_code/developer_options_code_buildflags.h"
#include "build/buildflag.h"

/// The code required to enter the developer menu in official builds
NSString* const kBraveDeveloperOptionsCode =
    @BUILDFLAG(BRAVE_IOS_DEVELOPER_OPTIONS_CODE);
