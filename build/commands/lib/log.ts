// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import os from 'node:os'
import chalk from 'chalk'
import logUpdate from 'log-update'
import tsm from 'teamcity-service-messages'
import { isTeamcity } from './ciDetect.ts'

export let divider: string
function setLineLength() {
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
const progressScopes: string[] = []

if (isTeamcity) {
  tsm.autoFlowId = false
  // Ensure that the output is not buffered when using Teamcity Service
  // Messages. If it is buffered, console.log() output may not display in
  // real-time, leading to a mix-up of service messages and external process
  // outputs.
  //
  // This is a known Node.js fix for issues caused by *Sync functions blocking
  // the event loop. Ideally, we should switch to async/await and wait for the
  // "drain" event.
  // @ts-ignore
  process.stdout._handle.setBlocking(true)
  // @ts-ignore
  process.stderr._handle.setBlocking(true)

  process.on('exit', () => {
    while (progressScopes.length) {
      tsm.blockClosed({ name: progressScopes.pop()! })
    }
  })
}

export function progressStart(message: string) {
  if (isTeamcity) {
    tsm.blockOpened({ name: message })
    progressScopes.push(message)
  } else {
    console.log(progressStyle(`${message}...`))
  }
}

export function progressFinish(message: string) {
  if (isTeamcity) {
    progressScopes.pop()
    tsm.blockClosed({ name: message })
  } else {
    console.log(progressStyle(`...${message} done`))
  }
}

export function progressScope(message: string, callable: () => void) {
  progressStart(message)
  try {
    callable()
  } finally {
    progressFinish(message)
  }
}

export async function progressScopeAsync(
  message: string,
  callable: () => Promise<void>,
) {
  progressStart(message)
  try {
    await callable()
  } finally {
    progressFinish(message)
  }
}

export function status(message: string, alreadyStyled = false) {
  if (isTeamcity) {
    tsm.progressMessage(message)
  } else {
    console.log(alreadyStyled ? message : statusStyle(message))
  }
}

export function fatal(message: string, code?: number | string | null): never {
  error(message)
  process.exit(code ?? 1)
}

export function error(message: string) {
  if (isTeamcity) {
    tsm.message({ text: message, status: 'ERROR' })
  } else {
    console.error(errorStyle(message))
  }
}

export function warn(message: string) {
  if (isTeamcity) {
    tsm.message({ text: message, status: 'WARNING' })
  } else {
    console.error(warningStyle(message))
  }
}

export function updateStatus(
  projectUpdateStatus: { name: string; ref: string; phase: string }[],
) {
  const statusLines = Object.values(projectUpdateStatus).map(
    (entry) =>
      `${chalk.bold(entry.name)} (${entry.ref}): ${chalk.green.italic(
        entry.phase,
      )}`,
  )
  logUpdate(statusLines.join(os.EOL))
}

export function command(dir: string, cmd: string, args: string[]) {
  console.log(divider)
  if (dir) {
    console.log(cmdDirStyle(dir))
  }
  status(`${cmdArrowStyle('>')} ${cmdCmdStyle(cmd)} ${args.join(' ')}`, true)
}
