// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/resource/resource_bundle.h"

#define AddDataPackFromPath(...)                                    \
  AddDataPackFromPath(                                              \
      pak_path.AppendASCII("brave_components_tests_resources.pak"), \
      ui::kScaleFactorNone);                                        \
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(__VA_ARGS__)

#include "src/components/test/components_test_suite.cc"

#undef AddDataPackFromPath
