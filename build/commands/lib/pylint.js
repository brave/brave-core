// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs')
const config = require('../lib/config')
const util = require('../lib/util')

const getDefaultOptions = () => {
  let options = Object.assign({}, config.defaultOptions)
  options.cwd = path.resolve(__dirname, '../../..')
  return options
}

const runPylint = (args, continueOnFail = false) => {
  let cmd_options = getDefaultOptions()
  cmd_options.continueOnFail = continueOnFail
  if (process.platform !== 'win32') {
    util.run('which', ['pylint'], cmd_options)
  } else {
    util.run('where', ['pylint'], cmd_options)
  }
  util.run('pylint', ['--version'], cmd_options)
  util.run('pylint', args, cmd_options)
}

const runGit = (gitArgs) => {
  // Have to use shell because otherwise git gets confused with node's escaping
  // of the args.
  let prog = util.run('git', gitArgs, { shell: true })
  const output = prog.stdout.toString()
  console.log(output)
  return output.trim()
}

const getChangedFiles = (base_branch, check_folders) => {
  const merge_base = runGit(['merge-base', 'origin/' + base_branch, 'HEAD'])
  if (!merge_base) {
    console.log('Could not determine merge-base.')
    process.exit(1)
  }
  const changed_files = runGit(['diff', '--name-only', '--diff-filter',
    'drt', merge_base, '--', check_folders.join('/*.py ') + '/*.py'])
  if (!changed_files) {
    return []
  }
  return changed_files.split('\n')
}

const pylint = (options = {}) => {
  const check_folders = ['build', 'components', 'installer', 'script', 'tools']
  const report_file = 'pylint-report.txt'

  let paths = check_folders
  let description = 'pylint findings'
  if (options.base) {
    paths = getChangedFiles(options.base, check_folders)
    description += ' in scripts changed relative to ' + options.base
  }
  description += ' in: ' + check_folders.join('/ ') + '/'

  if (!paths.length) {
    console.log('No ' + description)
    if (options.report) {
      try {
        fs.writeFileSync(report_file, '\n')
      } catch(err) {
        console.log('Unable to write to report file: ' + err)
      }
    }
    return
  }

  let args = ['-j0', '-rn', '--rcfile=.pylintrc']
  if (options.report) {
    args.push('-fparseable')
  }
  args = args.concat(paths)
  if (options.report) {
    args.push('>' + report_file)
  } 

  console.log('Checking for ' + description)
  runPylint(args, options.report)
}

module.exports = pylint
