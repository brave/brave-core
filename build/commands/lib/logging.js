// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const os = require('os')
const chalk = require('chalk')
const logUpdate = require('log-update')
const tsm =
  process.env.TEAMCITY_VERSION !== undefined
    ? require('teamcity-service-messages')
    : undefined

let divider
function setLineLength () {
  divider = Array(process.stdout.columns || 32).join('-')
}
setLineLength()
process.stdout.on('resize', setLineLength)

const progressStyle = chalk.bold.inverse
const statusStyle = chalk.green.italic
const warningStyle = chalk.black.bgYellow
const errorStyle = chalk.red

const cmdDirStyle = chalk.blue
const cmdCmdStyle = chalk.green
const cmdArrowStyle = chalk.magenta

// Track Teamcity progress scopes and finish them on unexpected exit.
const progressScopes = []

if (tsm) {
  tsm.autoFlowId = false
  // Ensure that the output is not buffered when using Teamcity Service
  // Messages. If it is buffered, console.log() output may not display in
  // real-time, leading to a mix-up of service messages and external process
  // outputs.
  //
  // This is a known Node.js fix for issues caused by *Sync functions blocking
  // the event loop. Ideally, we should switch to async/await and wait for the
  // "drain" event.
  process.stdout._handle.setBlocking(true)
  process.stderr._handle.setBlocking(true)

  process.on('exit', () => {
    while (progressScopes.length) {
      tsm.blockClosed({ name: progressScopes.pop() })
    }
  })
}

function progressStart(message) {
  if (tsm) {
    tsm.blockOpened({ name: message })
    progressScopes.push(message)
  } else {
    console.log(progressStyle(`${message}...`))
  }
}

function progressFinish(message) {
  if (tsm) {
    progressScopes.pop()
    tsm.blockClosed({ name: message })
  } else {
    console.log(progressStyle(`...${message} done`))
  }
}

function progressScope(message, callable) {
  progressStart(message)
  try {
    callable()
  } finally {
    progressFinish(message)
  }
}

async function progressScopeAsync(message, callable) {
  progressStart(message)
  try {
    await callable()
  } finally {
    progressFinish(message)
  }
}

function status(message, alreadyStyled = false) {
  if (tsm) {
    tsm.progressMessage(message)
  } else {
    console.log(alreadyStyled ? message : statusStyle(message))
  }
}

function error(message) {
  if (tsm) {
    tsm.message({text: message, status: 'ERROR'})
  } else {
    console.error(errorStyle(message))
  }
}

function warn(message) {
  if (tsm) {
    tsm.message({text: message, status: 'WARNING'})
  } else {
    console.error(warningStyle(message))
  }
}

function updateStatus (projectUpdateStatus) {
  const statusLines = Object.values(projectUpdateStatus).map(entry =>
    `${chalk.bold(entry.name)} (${entry.ref}): ${chalk.green.italic(entry.phase)}`
  )
  logUpdate(statusLines.join(os.EOL))
}

function command (dir, cmd, args) {
  console.log(divider)
  if (dir)
    console.log(cmdDirStyle(dir))
  status(`${cmdArrowStyle('>')} ${cmdCmdStyle(cmd)} ${args.join(' ')}`, true)
}

function printFailedPatchesInJsonFormat(allPatchStatus, bravePath) {
  const failedPatches = allPatchStatus.filter((patch) => patch.error)
  if (!failedPatches.length) {
    return;
  }

  const patchFailuresOutput = failedPatches.map(({path, patchPath}) => {
    return {
      // Trimming the patch path to be relative to the brave core repo. We skip
      // the first character to avoid the trailing slash from the absolute
      // path.
      patchPath:
        patchPath.replace(bravePath, '').substring(1),
      path: path
    }
  })

  console.log(chalk.red(`${failedPatches.length} Failed patches json breakdown:`))
  console.log(JSON.stringify(patchFailuresOutput))
  console.log(divider)
}

function allPatchStatus(allPatchStatus, patchGroupName) {
  if (!allPatchStatus.length) {
    console.log(chalk.bold.italic(`There were no ${patchGroupName} code patch updates to apply.`))
  } else {
    const successfulPatches = []
    const failedPatches = []
    for (const patchStatus of allPatchStatus) {
      if (!patchStatus.error) {
        successfulPatches.push(patchStatus)
      } else {
        failedPatches.push(patchStatus)
      }
    }
    console.log(chalk.bold(`There were ${allPatchStatus.length} ${patchGroupName} code patch updates to apply.`))
    if (successfulPatches.length) {
      console.log(chalk.green(`${successfulPatches.length} successful patches:`))
      successfulPatches.forEach(logPatchStatus)
    }
    if (failedPatches.length) {
      console.log(chalk.red(`${failedPatches.length} failed patches:`))
      failedPatches.forEach(logPatchStatus)
    }
  }
}

function logPatchStatus ({ reason, path, patchPath, error, warning }) {
  const GitPatcher = require('./gitPatcher')
  const success = !error
  const statusColor = success ? chalk.green : chalk.red
  console.log(statusColor.bold.underline(path || patchPath))
  console.log(`  - Patch applied because: ${GitPatcher.patchApplyReasonMessages[reason]}`)
  if (error) {
    console.log(chalk.red(`  - Error - ${error.message}`))
  }
  if (warning) {
    console.warn(chalk.yellow(`  - Warning - ${warning}`))
  }
  if (error)  {
    if (error.stdout) {
      console.log(chalk.blue(error.stdout))
    }
    if (error.stderr) {
      console.error(chalk.red(error.stderr))
    }
  }
  console.log(divider)
}

module.exports = {
  progressStart,
  progressFinish,
  progressScope,
  progressScopeAsync,
  status,
  error,
  warn,
  command,
  updateStatus,
  printFailedPatchesInJsonFormat,
  allPatchStatus
}
