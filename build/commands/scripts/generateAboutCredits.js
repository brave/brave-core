// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const build = require('../lib/build')
const config = require('../lib/config')

build(config.defaultBuildConfig, {
  C: 'generate_about_credits',
  target: 'components/resources:about_credits',
  gn: ['generate_about_credits:true'],
})
