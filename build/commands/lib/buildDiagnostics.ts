// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
import * as Log from './log.ts'
import { execSync } from 'node:child_process'

const kReadFileTailLimitBytes = 128 * 1024

/** Seconds to wait after SIGUSR2 before reading Node diagnostic report files. */
const kNodeReportWriteSettleSeconds = 8

/**
 * Dumps the hanged build diagnostics for a build.
 * @param {string} outputDir
 */
export function dumpBuildHangDiagnostics(outputDir: string) {
  const relativePaths = ['siso.INFO', 'siso.exe.INFO', 'siso_output']
  for (const rel of relativePaths) {
    let filePath = path.join(outputDir, rel)
    if (!fs.existsSync(filePath)) {
      const filePathRedirected = path.join(outputDir, rel + '.redirected')
      if (fs.existsSync(filePathRedirected)) {
        // read the redirected file and get the path
        const redirectedContent = fs.readFileSync(filePathRedirected, 'utf8')
        const redirectedPath = redirectedContent.trim()
        if (fs.existsSync(redirectedPath)) {
          filePath = redirectedPath
        } else {
          Log.error(`Redirected file ${redirectedPath} does not exist`)
          continue
        }
      } else {
        continue
      }
    }
    console.log(`--- tail of ${filePath} ---`)
    try {
      console.log(readFileTailUtf8Sync(filePath, kReadFileTailLimitBytes))
    } catch (err) {
      Log.error(err instanceof Error ? err.message : String(err))
    }
  }
}

/**
 * Reads only the last `maxBytes` bytes from a file.
 */
function readFileTailUtf8Sync(filePath: string, maxBytes: number): string {
  const fd = fs.openSync(filePath, 'r')
  try {
    const size = fs.fstatSync(fd).size
    if (size === 0) {
      return ''
    }
    const toRead = Math.min(maxBytes, size)
    const buf = Buffer.alloc(toRead)
    fs.readSync(fd, buf, 0, toRead, size - toRead)
    // May start mid-sequence for UTF-8; Node replaces invalid bytes at slice
    // edge.
    let str = buf.toString('utf8')
    if (toRead < size) {
      const nl = str.indexOf('\n')
      if (nl !== -1) {
        str = str.slice(nl + 1)
      }
    }
    return str
  } finally {
    fs.closeSync(fd)
  }
}

function psLineLooksLikeNodeProcess(line: string): boolean {
  console.log(`psLineLooksLikeNodeProcess: ${line}`)
  return /node-linux-x64\/bin\/node/.test(line)
}

function triggerNodeDiagnosticReportsAndPrint(
  outputDir: string,
  nodePids: string[],
) {
  if (!fs.existsSync(outputDir)) {
    return
  }

  const beforeMs = Date.now()

  if (nodePids.length === 0) {
    console.log(
      '--- Node diagnostic reports: no PIDs with --report-on-signal in environ (skipping SIGUSR2) ---',
    )
    return
  }

  console.log(
    `--- Requesting Node diagnostic reports (SIGUSR2) for PIDs: ${nodePids.join(', ')} ---`,
  )
  for (const pid of nodePids) {
    try {
      execSync(`kill -USR2 ${Number(pid)}`, {
        encoding: 'utf-8',
        stdio: 'pipe',
      })
    } catch (e) {
      const msg = e instanceof Error ? e.message : String(e)
      console.log(`kill -USR2 ${pid} failed: ${msg}`)
    }
  }

  try {
    execSync(`sleep ${kNodeReportWriteSettleSeconds}`, { stdio: 'ignore' })
  } catch {
    // ignore
  }

  let names: string[]
  try {
    names = fs.readdirSync(outputDir)
  } catch (e) {
    Log.error(e instanceof Error ? e.message : String(e))
    return
  }

  const reportFiles = names.filter(
    (n) => n.startsWith('report.') && n.endsWith('.json'),
  )
  const recentReports: string[] = []
  const skewMs = 2000
  for (const name of reportFiles) {
    const fp = path.join(outputDir, name)
    try {
      const st = fs.statSync(fp)
      if (st.mtimeMs >= beforeMs - skewMs) {
        recentReports.push(fp)
      }
    } catch {
      // ignore missing / race
    }
  }
  recentReports.sort()

  if (recentReports.length === 0) {
    console.log(
      `--- Node diagnostic reports: no new report.*.json under ${outputDir} after ${kNodeReportWriteSettleSeconds}s wait ---`,
    )
    return
  }

  for (const fp of recentReports) {
    console.log(`--- Node diagnostic report: ${fp} ---`)
    try {
      console.log(fs.readFileSync(fp, 'utf8'))
    } catch (e) {
      Log.error(e instanceof Error ? e.message : String(e))
    }
  }
}

/**
 * Dumps the hang diagnostics for the processes that are suspicious of being the
 * cause of the build hang. This is a best-effort attempt to diagnose the hang.
 * @param outputDir GN output directory; when set on Linux, triggers SIGUSR2 for
 * node processes that have `--report-on-signal` and prints new report JSONs.
 */
export function dumpProcessHangDiagnostics(outputDir?: string) {
  // Restrict to Linux for now.
  if (process.platform !== 'linux') {
    return
  }

  try {
    // Find all relevant processes
    const psCommand = 'ps -e -o pid,ppid,pcpu,pmem,stat,command'

    const psOutput = execSync(psCommand, { encoding: 'utf-8' })
    const lines = psOutput.split('\n')

    const suspiciousProcesses = lines.filter((line) => {
      const lower = line.toLowerCase()
      return (
        lower.includes('node')
        || lower.includes('python')
        || lower.includes('siso')
      )
    })

    console.log(
      `--- Suspicious Processes (PID, PPID, %CPU, %MEM, STAT, CMD) ---`,
    )
    for (const process of suspiciousProcesses) {
      console.log(process)
    }

    const looksLikeNumericPid = (pid: string | undefined): pid is string =>
      Boolean(pid && !isNaN(Number(pid)))

    // Extract just the PIDs of the processes to probe them further
    const pidsToProbe = suspiciousProcesses
      .map((line) => line.trim().split(/\s+/)[0])
      .filter(looksLikeNumericPid)

    const nodePids = suspiciousProcesses
      .filter(psLineLooksLikeNodeProcess)
      .map((line) => line.trim().split(/\s+/)[0])
      .filter(looksLikeNumericPid)

    console.log(`--- Deep Dive into PIDs: ${pidsToProbe.join(', ')} ---`)

    if (outputDir) {
      triggerNodeDiagnosticReportsAndPrint(outputDir, nodePids)
    }
    process.exit(1)

    // Allow gathering ptrace information. CI currently doesn't restrict sudo.
    try {
      execSync('sudo sysctl -w kernel.yama.ptrace_scope=0', {
        encoding: 'utf-8',
        stdio: 'inherit',
      })
    } catch (e) {
      console.log(`Error setting ptrace_scope: ${e}`)
    }
    for (const pid of pidsToProbe) {
      console.log(`\n>> Profiling PID: ${pid} <<`)
      // Grab 2 seconds of strace to see if it's spinning or deadlocked
      // We use timeout to ensure strace doesn't block our runner
      let straceOut = ''
      try {
        straceOut = execSync(
          `timeout 2 strace -f -y -tt -p ${pid} -s 256 2>&1`,
          {
            encoding: 'utf-8',
          },
        )
        console.log(`Strace summary (2s):\n${straceOut}`)
      } catch (e) {
        straceOut = `${e.stdout || ''}\n${e.stderr || ''}`
        console.log(`Strace summary (2s):\n${straceOut}`)
      }
    }
  } catch (e) {
    console.log(`Error during diagnostic collection: ${e}`)
  }
}
