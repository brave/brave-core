/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)

// Without this override there are error on Android cr130 build:
// password_store_built_in_backend.cc:217:36: error: use of undeclared
// identifier 'features'; did you mean 'sql::features'?
//   217 |       base::FeatureList::IsEnabled(features::kUseNewEncryptionMethod)
//       |                                    ^~~~~~~~
//       |                                    sql::features
// ../../sql/sql_features.h:11:16: note: 'sql::features' declared here
//    11 | namespace sql::features {
//       |                ^
// password_store_built_in_backend.cc:217:46: error: no member named
// 'kUseNewEncryptionMethod' in namespace 'sql::features'
//   217 |       base::FeatureList::IsEnabled(features::kUseNewEncryptionMethod)
//       |                                    ~~~~~~~~~~^
//
// The mentioned enums are at
// "components/password_manager/core/browser/features/password_features.h",
// which is included under pre-processor guards: #if
// !BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND) #include
// "components/password_manager/core/browser/features/password_features.h"
// #endif  // !BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
// #if !BUILDFLAG(IS_ANDROID)
// #include
// "components/password_manager/core/browser/features/password_features.h"
// #endif  // !BUILDFLAG(IS_ANDROID)
// Brave set `args.use_login_database_as_backend = true` to use Chromium's
// password manager
//
// Related Chromium change:
// https://source.chromium.org/chromium/chromium/src/+/70fb340855605469deb626635cdfee00e3d8caf8

#include "components/password_manager/core/browser/features/password_features.h"

#endif  // BUILDFLAG(IS_ANDROID)

#include "src/components/password_manager/core/browser/password_store/password_store_built_in_backend.cc"
