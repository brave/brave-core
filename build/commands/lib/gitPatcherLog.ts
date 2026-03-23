// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import chalk from 'chalk'
import * as Log from './log.ts'
import { patchApplyReasonMessages, getReasonName } from './gitPatcher.js'

export function printFailedPatchesInJsonFormat(
  allPatchStatus: {
    path: string
    patchPath: string
    reason: string
    error?: { message: string; stdout?: string; stderr?: string }
  }[],
  bravePath: string,
) {
  const failedPatches = allPatchStatus.filter((patch) => patch.error)
  if (!failedPatches.length) {
    return
  }

  const patchFailuresOutput = failedPatches.map(
    ({ path, patchPath, reason }) => {
      return {
        // Trimming the patch path to be relative to the brave core repo. We skip
        // the first character to avoid the trailing slash from the absolute
        // path.
        patchPath: patchPath.replace(bravePath, '').substring(1),
        reason: getReasonName(reason),
        path,
      }
    },
  )

  console.log(
    chalk.red(`${failedPatches.length} Failed patches json breakdown:`),
  )
  console.log(JSON.stringify(patchFailuresOutput))
  console.log(Log.divider)
}

export function allPatchStatus(
  allPatchStatus: {
    path: string
    patchPath: string
    reason: string
    error?: { message: string; stdout?: string; stderr?: string }
  }[],
  patchGroupName: string,
) {
  if (!allPatchStatus.length) {
    console.log(
      chalk.bold.italic(
        `There were no ${patchGroupName} code patch updates to apply.`,
      ),
    )
  } else {
    const successfulPatches: any[] = []
    const failedPatches: any[] = []
    for (const patchStatus of allPatchStatus) {
      if (!patchStatus.error) {
        successfulPatches.push(patchStatus)
      } else {
        failedPatches.push(patchStatus)
      }
    }
    console.log(
      chalk.bold(
        `There were ${allPatchStatus.length} ${patchGroupName} code patch updates to apply.`,
      ),
    )
    if (successfulPatches.length) {
      console.log(
        chalk.green(`${successfulPatches.length} successful patches:`),
      )
      successfulPatches.forEach(logPatchStatus)
    }
    if (failedPatches.length) {
      console.log(chalk.red(`${failedPatches.length} failed patches:`))
      failedPatches.forEach(logPatchStatus)
    }
  }
}

function logPatchStatus({ reason, path, patchPath, error, warning }) {
  const success = !error
  const statusColor = success ? chalk.green : chalk.red
  console.log(statusColor.bold.underline(path || patchPath))
  console.log(`  - Patch applied because: ${patchApplyReasonMessages[reason]}`)
  if (error) {
    console.log(chalk.red(`  - Error - ${error.message}`))
  }
  if (warning) {
    console.warn(chalk.yellow(`  - Warning - ${warning}`))
  }
  if (error) {
    if (error.stdout) {
      console.log(chalk.blue(error.stdout))
    }
    if (error.stderr) {
      console.error(chalk.red(error.stderr))
    }
  }
  console.log(Log.divider)
}
