// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Serves the Brave on Blink slides and drives the simulator demo.
// Run from src/brave: node brave_blink_slides/server.ts

import { spawn, execFile } from 'node:child_process'
import type { ChildProcess } from 'node:child_process'
import { once } from 'node:events'
import { mkdtempSync, readFileSync, rmSync } from 'node:fs'
import { readFile } from 'node:fs/promises'
import { createServer, get } from 'node:http'
import type { IncomingMessage, ServerResponse } from 'node:http'
import { tmpdir } from 'node:os'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { promisify } from 'node:util'
import ts from 'typescript'

const PORT = 8940
const SIM = 'B414BC85-5A80-4803-B360-596083E53F9F'
const SLIDES_DIR = dirname(fileURLToPath(import.meta.url))
const BRAVE_DIR = dirname(SLIDES_DIR)
const SHOWCASE = 'http://localhost:8932/'
const LION_SVG = join(
  BRAVE_DIR,
  'ios/brave-ios/Sources/BraveUI/Images/Shared.xcassets',
  'Colored PDF/brave.logo.imageset/brave.logo.svg',
)

// The only pages the slides may open in the simulator.
const TARGETS: Record<string, string> = {
  showcase: SHOWCASE,
  apis: `${SHOWCASE}?apis`,
  proof: `${SHOWCASE}?proof`,
  shields: `${SHOWCASE}?shields`,
  farbling: `${SHOWCASE}?farbling`,
  farblingOtherSite: 'http://127.0.0.1:8932/?farbling',
}

const run = promisify(execFile)
const screenDir = mkdtempSync(join(tmpdir(), 'brave-blink-slides-'))
const screenFile = join(screenDir, 'screen.jpg')

let demo: ChildProcess | undefined
let launching: Promise<void> | undefined
let demoLog: string[] = []
const logListeners = new Set<ServerResponse>()

function appendLog(chunk: string) {
  for (const line of chunk.split('\n')) {
    if (!line) {
      continue
    }
    demoLog.push(line)
    for (const res of logListeners) {
      res.write(`data: ${JSON.stringify(line)}\n\n`)
    }
  }
  demoLog = demoLog.slice(-200)
}

// Uses node:http on IPv4, since fetch() to localhost crashed the process with
// an uncaught setTypeOfService EINVAL.
function showcaseUp() {
  return new Promise<boolean>((resolve) => {
    const req = get({ host: '127.0.0.1', port: 8932, path: '/', timeout: 1000 })
    req.on('response', (res) => {
      res.resume()
      resolve(res.statusCode === 200)
    })
    req.on('timeout', () => req.destroy())
    req.on('error', () => resolve(false))
  })
}

// Always a full launch, so the simulator and app come up even if they were
// closed while the showcase kept serving.
async function launchDemo() {
  if (demo) {
    await stopDemo()
  }
  if (await showcaseUp()) {
    appendLog(`Another demo is already serving ${SHOWCASE}; stop it first.`)
    return
  }
  demoLog = []
  appendLog('Launching Brave in the simulator...')
  // Brave opens as on any launch; the demo buttons open the showcase later.
  demo = spawn('./brave_blink_demo.py', ['--serve'], {
    cwd: BRAVE_DIR,
    env: { ...process.env, PYTHONUNBUFFERED: '1' },
  })
  demo.stdout?.setEncoding('utf8').on('data', appendLog)
  demo.stderr?.setEncoding('utf8').on('data', appendLog)
  demo.on('exit', (code) => {
    appendLog(`demo exited (${code ?? 'signal'})`)
    demo = undefined
  })
}

// Repeated clicks share one launch instead of racing each other.
function launchOnce() {
  launching ??= launchDemo().finally(() => (launching = undefined))
  return launching
}

// The demo buttons work even if the demo was stopped or the simulator closed.
async function ensureDemo() {
  if (await showcaseUp()) {
    await run('xcrun', ['simctl', 'bootstatus', SIM, '-b'])
    return
  }
  if (!demo) {
    await launchOnce()
  }
  // The script only answers once Brave is up and its first tab is open.
  for (let tries = 0; tries < 120; tries++) {
    if (await showcaseUp()) {
      return
    }
    if (!demo) {
      break
    }
    await new Promise((resolve) => setTimeout(resolve, 1000))
  }
  appendLog('The demo did not start.')
  throw new Error('the demo did not start')
}

async function stopDemo() {
  const running = demo
  if (!running) {
    return
  }
  const exited = once(running, 'exit')
  running.kill('SIGINT')
  const timer = setTimeout(() => running.kill('SIGKILL'), 3000)
  await exited
  clearTimeout(timer)
}

async function openInSimulator(target: string) {
  if (!TARGETS[target]) {
    throw new Error(`unknown target ${target}`)
  }
  // A fresh URL makes the app open a new tab and refetch every request.
  const fresh = new URL(TARGETS[target])
  fresh.searchParams.set('t', String(Date.now()))
  const url = fresh.href
  appendLog(`open ${url}`)
  try {
    await run('xcrun', [
      'simctl',
      'openurl',
      SIM,
      `brave-debug://open-url?url=${encodeURIComponent(url)}`,
    ])
  } catch (e) {
    appendLog(`open failed: ${(e as { stderr?: string }).stderr?.trim() || e}`)
    throw e
  }
}

// Serialised so a slow capture never overlaps the next poll.
let capture: Promise<Buffer | undefined> | undefined
function captureScreen() {
  capture ??= run('xcrun', [
    'simctl',
    'io',
    SIM,
    'screenshot',
    '--type=jpeg',
    screenFile,
  ])
    .then(() => readFile(screenFile))
    .catch(() => undefined)
    .finally(() => (capture = undefined))
  return capture
}

function transpileClient(): string {
  const source = readFileSync(join(SLIDES_DIR, 'slides.ts'), 'utf8')
  return ts.transpileModule(source, {
    compilerOptions: {
      target: ts.ScriptTarget.ES2022,
      module: ts.ModuleKind.ESNext,
    },
  }).outputText
}

// The lion's shield and face, stroked instead of filled.
function lionFrame(): string {
  const svg = readFileSync(LION_SVG, 'utf8')
  const [outline = '', ...features] = [...svg.matchAll(/<path[^>]* d="([^"]+)"/g)]
    .map((match) => match[1])
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="3 1.5 18 21"
  fill="none" stroke-linejoin="round" stroke-linecap="round">
  <path class="frame-outer" d="${outline}" stroke="#3b6fe0" stroke-width="0.22"/>
  <path class="frame-inner" d="${outline}" stroke="#7fa3f2" stroke-width="0.08"
    transform="translate(12 12) scale(0.9) translate(-12 -12)"/>
  ${features.map((d) => `<path class="feature" d="${d}" stroke="#3b6fe0" stroke-width="0.14"/>`).join('\n  ')}
</svg>`
}

const STATIC: Record<string, [string, string]> = {
  '/': ['index.html', 'text/html; charset=utf-8'],
  '/slides.css': ['slides.css', 'text/css; charset=utf-8'],
}

function send(res: ServerResponse, status: number, type: string, body: string | Buffer) {
  res.writeHead(status, { 'Content-Type': type, 'Cache-Control': 'no-store' })
  res.end(body)
}

async function handle(req: IncomingMessage, res: ServerResponse) {
  const url = new URL(req.url ?? '/', `http://${req.headers.host}`)
  const path = url.pathname

  if (req.method === 'GET') {
    const file = STATIC[path]
    if (file) {
      return send(res, 200, file[1], readFileSync(join(SLIDES_DIR, file[0])))
    }
    if (path === '/slides.js') {
      return send(res, 200, 'text/javascript; charset=utf-8', transpileClient())
    }
    if (path === '/lion-frame.svg') {
      return send(res, 200, 'image/svg+xml', lionFrame())
    }
    if (path === '/api/screen') {
      const jpeg = await captureScreen()
      return jpeg
        ? send(res, 200, 'image/jpeg', jpeg)
        : send(res, 503, 'text/plain', 'simulator not booted')
    }
    if (path === '/api/status') {
      return send(res, 200, 'application/json', JSON.stringify({ running: !!demo || (await showcaseUp()) }))
    }
    if (path === '/api/log') {
      res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-store',
      })
      for (const line of demoLog) {
        res.write(`data: ${JSON.stringify(line)}\n\n`)
      }
      logListeners.add(res)
      req.on('close', () => logListeners.delete(res))
      return
    }
  }

  // Other sites can't set this header without a CORS preflight, which is never
  // answered, so they can't drive the simulator.
  if (req.method === 'POST' && req.headers['x-brave-slides'] === '1') {
    if (path === '/api/launch') {
      await launchOnce()
      return send(res, 200, 'application/json', '{}')
    }
    if (path === '/api/stop') {
      appendLog(demo ? 'Stopping the demo...' : 'Stop: nothing running.')
      await stopDemo()
      return send(res, 200, 'application/json', '{}')
    }
    const open = path.match(/^\/api\/open\/(\w+)$/)
    if (open && TARGETS[open[1]]) {
      try {
        await ensureDemo()
        await openInSimulator(open[1])
        return send(res, 200, 'application/json', '{}')
      } catch (e) {
        return send(res, 400, 'text/plain', String(e))
      }
    }
  }

  send(res, 404, 'text/plain', 'not found')
}

const server = createServer((req, res) => {
  handle(req, res).catch((e) => send(res, 500, 'text/plain', String(e)))
})

// Loopback only: the API launches local processes.
server.listen(PORT, '127.0.0.1', () => {
  console.log(`Slides on http://localhost:${PORT}/ (Ctrl-C to stop)`)
})

function shutdown() {
  stopDemo()
  rmSync(screenDir, { recursive: true, force: true })
  process.exit(0)
}
process.on('SIGINT', shutdown)
process.on('SIGTERM', shutdown)
