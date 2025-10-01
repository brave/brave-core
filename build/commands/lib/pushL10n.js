/* Copyright (c) 2017 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const l10nUtil = require('./l10nUtil')

const pushL10n = (options) => {
  const runOptions = { cwd: config.srcDir }
  const cmdOptions = config.defaultOptions
  cmdOptions.cwd = config.braveCoreDir
  const extraScriptOptions = options.with_translations
    ? '--with_translations'
    : options.with_missing_translations
      ? '--with_missing_translations'
      : ''
  // Get rid of the copied from //brave xtb and grd changes.
  let args = ['checkout', '--', '*.xtb']
  util.run('git', args, runOptions)
  args = ['checkout', '--', '*.grd*']
  util.run('git', args, runOptions)

  l10nUtil.getBraveTopLevelPaths().forEach((sourceStringPath) => {
    if (
      !options.grd_path
      || sourceStringPath.endsWith(path.sep + options.grd_path)
    ) {
      args = [
        'script/push-l10n.py',
        '--channel',
        options.channel,
        '--source_string_path',
        sourceStringPath,
      ]
      if (extraScriptOptions) {
        args.push(extraScriptOptions)
      }
      util.run('python3', args, cmdOptions)
    }
  })
}

module.exports = pushL10n
