// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// We should always fork the typescript checker when we're in dev mode, as
// there's no disadvantage to doing so. Static builds use the ts-loader
// typechecker, so the build fails when typechecking fails.
// Note: argv will include 'dev' for dev builds and 'build' for static builds.
export const forkTsChecker = process.argv.includes('dev')
