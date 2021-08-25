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

const getDepotToolsPath = () => {
  const depotToolsPath = path.resolve(__dirname, '../../../vendor/depot_tools')
  if (!fs.existsSync(depotToolsPath)) {
    console.warn('Depot Tools path [' + depotToolsPath + '] doesn\'t exist')
    return ''
  }
  return depotToolsPath
}

const ensureDepotToolsInPath = (options) => {
  const depotToolsPath = getDepotToolsPath()
  if (!depotToolsPath) {
    return
  }
  let newPath = options.env.path
  if (!newPath) {
    process.env.Path && (newPath = process.env.Path)
    process.env.PATH && (newPath = process.env.PATH)
  }
  newPath = newPath.split(path.delimiter)
  if (!newPath.includes(depotToolsPath)) {
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

const parsePylintVersion = (file, deleteAfter = true) => {
  let data
  try {
    data = fs.readFileSync(file, 'utf8');
  } catch(err) {
    console.error('Unable to read file: ' + file + 'error: ', err);
    process.exit(1)
  }
  if (deleteAfter) {
    deleteFile(file)
  }
  console.log(data)
  const python = 'Python '
  return data.substr(data.indexOf(python) + python.length, 1)
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
  return parsePylintVersion(pylintVersionFile)
}

const runPylint = (args, continueOnFail = true) => {
  let cmd_options = getDefaultOptions()
  cmd_options.continueOnFail = continueOnFail
  const prog = util.run('pylint', args, cmd_options)
  return (prog.status === 0)
}

const installPylint3 = () => {
  let cmd_options = getDefaultOptions()
  cmd_options.continueOnFail = false
  const prog = util.runProcess(process.platform !== 'win32' ? 'which' : 'where',
    ['python3'], cmd_options)
  if (prog.status !== 0) {
    console.error('python3 could not be found in path')
    process.exit(1)
  }
  // Windows is the only platform that uses python from depot_tools. On other
  // platforms it's up to the user to install pylint in their python3 instance.
  if (process.platform === 'win32') {
    util.run('python3', ['-m', 'pip', 'install', 'pylint'], cmd_options)
  }
  util.run('python3', ['-m', 'pylint', '--version'], cmd_options)
}

const runPylint3 = (args, continueOnFail = true) => {
  let cmd_options = getDefaultOptions()
  cmd_options.continueOnFail = continueOnFail
  const prog = util.run('python3', ['-m', 'pylint'].concat(args), cmd_options)
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

const isPython3Script = (path) => {
  // Expecting python3 scripts to have #!/usr/bin/env python3 header
  let data
  try {
    data = fs.readFileSync(path, 'utf8');
  } catch(err) {
    console.error('Unable to read file: ' + path + 'error: ', err);
    process.exit(1)
  }
  return (data.split('\n', 2)[0].indexOf('python3') !== -1)
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

const filterScriptsByVersion = (paths) => {
  let py2_scripts = []
  let py3_scripts = []
  for (const path of paths) {
    if (isPython3Script(path)) {
      py3_scripts.push(path)
    } else {
      py2_scripts.push(path)
    }
  }
  return { py2_scripts, py3_scripts }
}

const runPylintLoop = (options, args, paths, report_file, func = runPylint) => {
  // On Windows, command line limit is 8192 chars, so may have to make multiple
  // calls to pylint. Leave some slack for initial args and redirect to report
  const maxCmdLineLength = 8000

  // Convenience funcion
  const doPylint = (loop_args) => {
    if (options.report) {
      loop_args.push('>>' + report_file)
    }
    return func(loop_args)
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

  // Print out which pylint is going to be used and its version
  const version_info = getPylintInfo()
  // If the above determined that python on the system is python3 then we can
  // run the same pylint on all changed files together. If, however, the system
  // defaults to python2, then we need to split out python3 scripts and use
  // python3 with its own pylint to check them.
  const is_python2_system = (version_info === '2')
  let filtered_paths = {}
  if (is_python2_system) {
    filtered_paths = filterScriptsByVersion(paths)
    if (filtered_paths.py3_scripts.length) {
      installPylint3()
    }
  } else {
    filtered_paths.py3_scripts = paths
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
  if (filtered_paths.py2_scripts.length) {
    let py2_args = [...args]
    py2_args.push('--rcfile=.pylint2rc')
    result &= runPylintLoop(options, py2_args, filtered_paths.py2_scripts, report_file)
  }

  if (filtered_paths.py3_scripts.length) {
    let py3_args = [...args]
    py3_args.push('--rcfile=.pylintrc')
    result &= runPylintLoop(options, py3_args, filtered_paths.py3_scripts,
      report_file, is_python2_system ? runPylint3 : runPylint)
  }

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
