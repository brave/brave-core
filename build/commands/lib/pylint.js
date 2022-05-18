// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs')
const config = require('../lib/config')
const util = require('../lib/util')
const {EOL} = require('os');

const deleteFile = (path) => {
  if (fs.existsSync(path)) {
    try {
      fs.unlinkSync(path)
    } catch(err) {
      console.error('Unable to delete file: ' + path + ' error: ', err)
      process.exit(1)
    }
  }
}

const getDefaultOptions = () => {
  let options = Object.assign({}, config.defaultOptions)
  // options.env.PYLINTRC = path.join(config.braveCoreDir, '.pylintrc')
  options.cwd = config.braveCoreDir
  return options
}

const runPylint = (args, continueOnFail = true) => {
  let cmd_options = getDefaultOptions()
  cmd_options.continueOnFail = continueOnFail
  const pylint = process.platform === 'win32' ? 'pylint-2.7.bat' : 'pylint-2.7'
  const prog = util.run(path.join(config.depotToolsDir, pylint), args, cmd_options)
  return (prog.status === 0)
}

const runGit = (gitArgs) => {
  // Have to use shell because otherwise git gets confused with node's escaping
  // of the args.
  let prog = util.run('git', gitArgs, { shell: true })
  const output = prog.stdout.toString()
  console.log(output)
  return output.trim()
}

const formatFolders = (folders, sep = '/*.py') => {
  return folders.join(sep + ' ') + sep
}

const createEmptyReportFile = (file_path) => {
  try {
    fs.writeFileSync(file_path, EOL)
  } catch(err) {
    console.error('Unable to write to file: ' + file_path + ' error: ', err)
    process.exit(1)
  }
}

const getAllFiles = (check_folders) => {
  const files = runGit(['ls-files', '--', formatFolders(check_folders)])
  if (!files) {
    return []
  }
  return files.split('\n')
}

const getChangedFiles = (base_branch, check_folders) => {
  if (!base_branch) {
    return getAllFiles(check_folders)
  }

  const merge_base = runGit(['merge-base', 'origin/' + base_branch, 'HEAD'])
  if (!merge_base) {
    console.error('Could not determine merge-base for branch ' + base_branch)
    process.exit(1)
  }
  const changed_files = runGit(['diff', '--name-only', '--diff-filter', 'drt',
    merge_base, '--', formatFolders(check_folders)])
  if (!changed_files) {
    return []
  }
  return changed_files.split('\n')
}

const getDescription = (base, check_folders) => {
  let description = 'pylint findings'
  if (base) {
    description += ' in scripts changed relative to ' + base
  }
  description += ' in: ' + formatFolders(check_folders, '/')
  return description
}

const runPylintLoop = (options, args, paths, report_file) => {
  // On Windows, command line limit is 8192 chars, so may have to make multiple
  // calls to pylint. Leave some slack for initial args and redirect to report
  const maxCmdLineLength = 8000

  // Convenience funcion
  const doPylint = (loop_args) => {
    if (options.report) {
      loop_args.push('>>' + report_file)
    }
    return runPylint(loop_args)
  }

  let result = true
  let currentLen = 0
  let loop_args = [...args]
  // Run pylint for each max command line length
  for (const pyPath of paths) {
    if (currentLen + pyPath.length > maxCmdLineLength) {
      result &= doPylint(loop_args)
      currentLen = 0
      loop_args = [...args]
    } 
    loop_args.push(pyPath)
    currentLen += pyPath.length
  }

  if (currentLen > 0) {
    result &= doPylint(loop_args) 
  }

  return result
}

const pylint = (options = {}) => {
  const check_folders = ['build', 'components', 'installer', 'script', 'tools']
  const report_file = 'pylint-report.txt'

  const description = getDescription(options.base, check_folders)

  // Get changed or all python files
  const paths = getChangedFiles(options.base, check_folders)
  if (!paths.length) {
    console.log('No ' + description)
    if (options.report) {
      createEmptyReportFile(report_file)
    }
    return
  }

  // Prepare pylint args
  let args = ['-j0', '-rn']
  if (options.report) {
    args.push('-fparseable')
    // Clean previous report as we will be appending to it
    deleteFile(report_file)
  }

  console.log('Checking for ' + description)

  let result = true

  let py3_args = [...args]
  result &= runPylintLoop(options, py3_args, paths, report_file)

  // When the report option is present we don't want to exit with an error so
  // that CI can continue and parse the report
  if (!result && !options.report) {
    process.exit(1)
  }

  // Make sure report file contails at least EOL, otherwise the Jenkins parsing
  // plugin will error out.
  if (options.report) {
    const stats = fs.statSync(report_file)
    if (stats.size === 0) {
      deleteFile(report_file)
      createEmptyReportFile(report_file)
    }
  }
}

module.exports = pylint
