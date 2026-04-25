// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/chrome_layout_provider.h"

// Replace LayoutProvider creation with our own function.
// The definition is provided similarly by the override of
// the header file.
#define CreateLayoutProvider CreateLayoutProvider_ChromiumImpl
#include <chrome/browser/ui/views/chrome_layout_provider.cc>
#undef CreateLayoutProvider

// CreateLayoutProvider() is defined at brave_layout_provider.cc
// to avoid brave dependencies.
