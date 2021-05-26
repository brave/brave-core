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

const ensureDepotToolsInPath = (options) => {
  const depotToolsPath = path.resolve(__dirname, '../../../vendor/depot_tools')
  if (!fs.existsSync(depotToolsPath)) {
    console.warn('Depot Tools path [' + depotToolsPath + '] doesn\'t exist')
    return
  }
  let newPath = options.env.path
  if (!newPath) {
    process.env.Path && (newPath = process.env.Path)
    process.env.PATH && (newPath = process.env.PATH)
  }
  newPath = newPath.split(path.delimiter)
  if (!newPath.includes(depotToolsPath)) {
    console.info('Prepending [' + depotToolsPath + '] to the path')
    newPath.unshift(depotToolsPath)
  }
  options.env.path = newPath.join(path.delimiter)
}

const getDefaultOptions = () => {
  let options = Object.assign({}, config.defaultOptions)
  options.cwd = path.resolve(__dirname, '../../..')
  ensureDepotToolsInPath(options)
  return options
}

const getPylintInfo = () => {
  const cmd_options = getDefaultOptions()
  // Use runProcess here because `which` fails silently (so `run` would not
  // print anything) and we want to show the error.
  const prog = util.runProcess(process.platform !== 'win32' ? 'which' : 'where',
    ['pylint'], cmd_options)
  if (prog.status !== 0) {
    console.error('pylint could not be found in path')
    process.exit(1)
  }

  // Get pylint version, and parse it to get the associated python version so
  // that we know which pylintrc config file to use. Can't seem to get this via
  // stdout so ended up redirecting to a file and then reading from it.
  const pylintVersionFile = 'pylint-version.txt'
  deleteFile(pylintVersionFile)
  util.run('pylint', ['--version', '>' + pylintVersionFile], cmd_options)
  let data
  try {
    data = fs.readFileSync(pylintVersionFile, 'utf8');
  } catch(err) {
    console.error('Unable to read file: ' + pylintVersionFile + 'error: ', err);
    process.exit(1)
  }
  deleteFile(pylintVersionFile)
  console.log(data)
  const python = 'Python '
  return data.substr(data.indexOf(python) + python.length, 1)
}

const runPylint = (args, continueOnFail = true) => {
  let cmd_options = getDefaultOptions()
  cmd_options.continueOnFail = continueOnFail
  const prog = util.run('pylint', args, cmd_options)
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

const createEmptyReportFile = (path) => {
  try {
    fs.writeFileSync(path, EOL)
  } catch(err) {
    console.error('Unable to write to file: ' + path + ' error: ', err)
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

const pylint = (options = {}) => {
  const check_folders = ['build', 'components', 'installer', 'script', 'tools']
  const report_file = 'pylint-report.txt'

  const description = getDescription(options.base, check_folders)

  // Print out which pylint is going to be used and its version
  const python_version = getPylintInfo()

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
  const rcfile = python_version === '2' ? '.pylint2rc' : '.pylintrc' 
  let args = ['-j0', '-rn', '--rcfile=' + rcfile]
  if (options.report) {
    args.push('-fparseable')
  }

  // On Windows, command line limit is 8192 chars, so may have to make multiple
  // calls to pylint. If that happens we will be adding to the same report file,
  // so make sure the file is clean. Leave some slack for initial args and
  // report file redirect
  const maxCmdLineLength = 8000
  if (options.report) {
    deleteFile(report_file)
  } 

  console.log('Checking for ' + description)

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

  // When the report option is present we don't want to exit with an error so
  // that CI can continue and parse the report
  if (!result && !options.report) {
    process.exit(1)
  }
}

module.exports = pylint
