// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const build = require('./build')

const buildFuzzer = (fuzzer_test_target, options) => {
  options.use_libfuzzer = true
  options.is_asan = true
  options.C = 'Fuzzer'
  options.target = fuzzer_test_target
  options.is_component_build = false
  options.is_debug = true

  build('Fuzzer', options)
}

module.exports = buildFuzzer
