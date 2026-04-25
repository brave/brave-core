// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import genTsConfig from '../lib/genTsConfig.js'

genTsConfig().catch((err) => {
  console.error(err)
  process.exit(1)
})
