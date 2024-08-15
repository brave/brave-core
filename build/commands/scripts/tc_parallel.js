// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const assert = require('assert')
const fs = require('fs')
const { spawn } = require('child_process')
const readline = require('readline')
const { EventEmitter } = require('events')
const tsm = require('teamcity-service-messages')

if (process.argv.length !== 3) {
  console.error('Usage: npm run tc_parallel <path-to-commands-file>')
  process.exit(1)
}

const commandsFilePath = process.argv[2]

if (!fs.existsSync(commandsFilePath)) {
  console.error(`File not found: ${commandsFilePath}`)
  process.exit(1)
}

const commands = fs
  .readFileSync(commandsFilePath, 'utf-8')
  .split('\n')
  .filter((line) => line.trim() !== '')

tsm.autoFlowId = false

const killEvent = new EventEmitter()
const closeBlockClosures = []

const runCommand = async (command, commandParams) => {
  const delay = commandParams.delay
  if (delay) {
    console.log(`Waiting for ${delay}s before starting next command...`)
    await new Promise((resolve) => setTimeout(resolve, delay * 1000))
  }

  return await new Promise((resolve) => {
    const name = commandParams.name || command
    const flowId = commandParams.flowId
    tsm.blockOpened({ name, flowId })

    let blockClosed = false
    const closeBlockClosure = () => {
      if (!blockClosed) {
        blockClosed = true
        tsm.blockClosed({ name, flowId })
      }
    }
    closeBlockClosures.push(closeBlockClosure)

    const child = spawn(command, {
      env: { ...process.env, TEAMCITY_FLOW_ID: flowId },
      shell: true
    })

    const rlStdout = readline.createInterface({
      input: child.stdout
    })
    const rlStderr = readline.createInterface({
      input: child.stderr
    })

    rlStdout.on('line', (line) => {
      if (line.startsWith('##teamcity')) {
        console.log(line)
      } else {
        tsm.message({ text: line, flowId: flowId })
      }
    })

    rlStderr.on('line', (line) => {
      if (line.startsWith('##teamcity')) {
        console.log(line)
      } else {
        tsm.message({ text: line, status: 'WARNING', flowId: flowId })
      }
    })

    let isCancelled = false
    killEvent.on('cancel', () => {
      isCancelled = true
      child.kill()
    })

    killEvent.on('signal', (signal) => {
      child.kill(signal)
    })

    child.on('exit', (code, signal) => {
      child.stdout.destroy()
      child.stderr.destroy()
      if (signal) {
        if (isCancelled) {
          tsm.message({
            text: `Process cancelled with ${signal}`,
            flowId: flowId
          })
        } else {
          tsm.buildProblem({
            description: `Process exited with ${signal}`,
            flowId: flowId
          })
        }
      } else if (code) {
        tsm.buildProblem({
          description: `Process exited with code ${code}`,
          flowId: flowId
        })
      }
      closeBlockClosure()
      resolve(!signal && !code)
    })

    child.on('error', (err) => {
      tsm.buildProblem({
        description: `Process launch error ${err}`,
        flowId: flowId
      })
      closeBlockClosure()
      resolve(false)
    })
  })
}

class Semaphore {
  constructor(max) {
    this.queue = []
    this.count = 0
    this.max = max
  }

  async acquire() {
    if (this.count < this.max) {
      this.count++
      return Promise.resolve()
    }
    return new Promise((resolve) => this.queue.push(resolve))
  }

  release() {
    this.count--
    if (this.queue.length > 0) {
      this.count++
      this.queue.shift()()
    }
  }
}

const runAllCommands = async () => {
  let semaphore = null
  let semaphoreMax = 4
  let tasks = []
  let flowId = Date.now()
  let nextCommandParams = {}

  for (const command of commands) {
    if (command.startsWith('#')) {
      continue
    }

    if (command.startsWith('parallel=')) {
      assert(!semaphore)
      semaphoreMax = parseInt(command.substring(command.indexOf('=') + 1))
      continue
    }

    if (command.startsWith('name=')) {
      nextCommandParams.name = command.substring(command.indexOf('=') + 1)
      continue
    }

    if (command.startsWith('delay=')) {
      nextCommandParams.delay = parseInt(
        command.substring(command.indexOf('=') + 1)
      )
      continue
    }

    if (!semaphore) {
      semaphore = new Semaphore(semaphoreMax)
    }

    nextCommandParams.flowId = flowId++

    const commandParams = nextCommandParams
    tasks.push(async () => {
      try {
        await semaphore.acquire()
        const runResult = await runCommand(command, commandParams)
        if (!runResult) {
          killEvent.emit('cancel')
        }
        return runResult
      } catch (err) {
        throw err
      } finally {
        semaphore.release()
      }
    })

    nextCommandParams = {}
  }

  try {
    const executeTasks = tasks.map((task) => task())
    const runResults = await Promise.all(executeTasks)
    process.exit(runResults.every((result) => result) ? 0 : 1)
  } catch (err) {
    killEvent.emit('cancel')
    throw err
  }
}

process.on('exit', () => {
  while (closeBlockClosures.length) {
    closeBlockClosures.pop()()
  }
})

const signalsToForward = ['SIGINT', 'SIGTERM', 'SIGQUIT', 'SIGHUP']
signalsToForward.forEach((signal) => {
  process.addListener(signal, () => killEvent.emit('signal', signal))
})

runAllCommands()
