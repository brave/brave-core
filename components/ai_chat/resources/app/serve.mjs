// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Dev server for the ai_chat_app standalone build.
// - Serves webpack output from dist/ai_chat_app/
// - Routes /conversation-entries/* to the iframe shell
// - Proxies /v1/* to the AI backend (avoids CORS)
// - Serves /static/* CSS+fonts from the GN build output
// - Serves /brave-icons/* from @brave/leo/icons
// - Serves /leo-variables.css from @brave/leo/tokens/css/variables.css
// - Generates /strings.js from source GRDP files

import http from 'http'
import https from 'https'
import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const BRAVE_DIR = path.resolve(__dirname, '../../../..')
const DIST_DIR = path.resolve(BRAVE_DIR, 'dist/ai_chat_app')

// ---------------------------------------------------------------------------
// GN build output (for CSS/fonts)
// ---------------------------------------------------------------------------

function findGenDir() {
  const candidates = [
    process.env.ROOT_GEN_DIR,
    path.resolve(BRAVE_DIR, '../out/Component/gen'),
    path.resolve(BRAVE_DIR, '../out/Debug/gen'),
    path.resolve(BRAVE_DIR, '../out/Default/gen'),
    path.resolve(BRAVE_DIR, '../out/Release/gen'),
  ].filter(Boolean)

  for (const d of candidates) {
    if (fs.existsSync(path.join(d, 'brave/build_flags.json'))) return d
  }
  return null
}

const ROOT_GEN_DIR = findGenDir()
const WEBUI_RESOURCES = ROOT_GEN_DIR
  ? path.join(ROOT_GEN_DIR, 'brave/ui/webui/resources')
  : null

// ---------------------------------------------------------------------------
// AI backend proxy
// ---------------------------------------------------------------------------

const AI_BACKEND = process.env.AI_BACKEND || 'http://127.0.0.1:8000'
const backendUrl = new URL(AI_BACKEND)
const backendIsHttps = backendUrl.protocol === 'https:'

function proxyToBackend(req, res) {
  const targetPath = req.url
  const options = {
    hostname: backendUrl.hostname,
    port: backendUrl.port || (backendIsHttps ? 443 : 80),
    path: targetPath,
    method: req.method,
    headers: { ...req.headers, host: backendUrl.host },
  }

  const transport = backendIsHttps ? https : http
  const proxyReq = transport.request(options, (proxyRes) => {
    // Forward CORS headers so the browser accepts the response
    const headers = {
      ...proxyRes.headers,
      'access-control-allow-origin': '*',
    }
    res.writeHead(proxyRes.statusCode, headers)
    proxyRes.pipe(res)
  })

  proxyReq.on('error', (err) => {
    console.error(`Backend proxy error: ${err.message}`)
    if (!res.headersSent) {
      res.writeHead(502, { 'Content-Type': 'text/plain' })
    }
    res.end(`Backend unavailable: ${err.message}`)
  })

  req.pipe(proxyReq)
}

// ---------------------------------------------------------------------------
// String extraction from GRDP source files
// ---------------------------------------------------------------------------

function parseGrdp(filePath) {
  const content = fs.readFileSync(filePath, 'utf8')
  const result = {}
  const msgRe = /<message\s[^>]*name="([^"]+)"[^>]*>([\s\S]*?)<\/message>/g
  let m
  while ((m = msgRe.exec(content)) !== null) {
    const key = m[1].replace(/^IDS_/, '')
    let text = m[2]
    // Collapse <ph name="...">content</ph> → content
    text = text.replace(/<ph[^>]*>([\s\S]*?)<\/ph>/g, '$1')
    // Strip remaining tags (e.g. <ex>)
    text = text.replace(/<[^>]+>/g, '')
    result[key] = text.trim()
  }
  return result
}

const GRDP_FILES = [
  path.join(BRAVE_DIR, 'components/resources/ai_chat_ui_strings.grdp'),
  path.join(BRAVE_DIR, 'components/resources/ai_chat_prompts.grdp'),
]

const AI_CHAT_STRINGS = Object.assign(
  {},
  ...GRDP_FILES.filter((f) => fs.existsSync(f)).map(parseGrdp),
)

// UI feature flags read by ai_chat_context.tsx at startup via loadTimeData.
const UI_FLAGS = {
  standalone: true,
  isMobile: false,
  isHistoryEnabled: true,
  isAIChatAgentProfileFeatureEnabled: false,
  isAIChatAgentProfile: false,
}

const STRINGS_JS =
  `loadTimeData.data = ${JSON.stringify({ ...UI_FLAGS, ...AI_CHAT_STRINGS }, null, 2)};`

// ---------------------------------------------------------------------------
// HTTP helpers
// ---------------------------------------------------------------------------

const MIME = {
  '.html':  'text/html; charset=utf-8',
  '.js':    'application/javascript; charset=utf-8',
  '.css':   'text/css; charset=utf-8',
  '.svg':   'image/svg+xml',
  '.png':   'image/png',
  '.jpg':   'image/jpeg',
  '.woff':  'font/woff',
  '.woff2': 'font/woff2',
  '.ttf':   'font/ttf',
  '.json':  'application/json',
  '.map':   'application/json',
}

function mime(p) {
  return MIME[path.extname(p).toLowerCase()] || 'application/octet-stream'
}

function serveFile(res, filePath) {
  if (!fs.existsSync(filePath)) {
    res.writeHead(404, { 'Content-Type': 'text/plain' })
    res.end(`Not found: ${filePath}`)
    return
  }
  res.writeHead(200, { 'Content-Type': mime(filePath) })
  fs.createReadStream(filePath).pipe(res)
}

function serveText(res, contentType, body) {
  res.writeHead(200, { 'Content-Type': contentType })
  res.end(body)
}

// ---------------------------------------------------------------------------
// loadTimeData stub (must be loaded before strings.js)
// ---------------------------------------------------------------------------

const LOAD_TIME_DATA_JS = `
window.loadTimeData = {
  data: {},
  getString(key) {
    return this.data[key] !== undefined ? String(this.data[key]) : key
  },
  getStringF(key, ...args) {
    let s = this.getString(key)
    args.forEach((arg, i) => { s = s.replaceAll('$' + (i + 1), arg) })
    return s
  },
  getBoolean(key)  { return Boolean(this.data[key]) },
  getInteger(key)  { return parseInt(this.data[key] || '0', 10) },
  getValue(key)    { return this.data[key] },
  valueExists(key) { return key in this.data },
  isInitialized()  { return true },
}
`

// ---------------------------------------------------------------------------
// Request handler
// ---------------------------------------------------------------------------

const LEO_ICONS = path.join(BRAVE_DIR, 'node_modules/@brave/leo/icons')
// Raw primitive token values — no browser-theme var() references.
const LEO_VARIABLES_CSS = path.join(BRAVE_DIR, 'node_modules/@brave/leo/tokens/css/variables.css')
// Font + typography tokens.
const LEO_BROWSER_CSS = path.join(BRAVE_DIR, 'node_modules/@brave/leo/tokens/css/variables-browser.css')
const PORT = parseInt(process.env.PORT || '3000', 10)

const server = http.createServer((req, res) => {
  const { pathname } = new URL(req.url, `http://localhost:${PORT}`)

  // ── AI backend proxy ─────────────────────────────────────────────────────
  // Handles CORS-free proxying of all API requests to the AI backend.
  if (pathname.startsWith('/v1/')) {
    return proxyToBackend(req, res)
  }

  // ── Conversation entries iframe ──────────────────────────────────────────
  if (pathname.startsWith('/conversation-entries/')) {
    return serveFile(res, path.join(__dirname, 'conversation-entries.html'))
  }

  // ── Chrome WebUI static resources (CSS, fonts) ───────────────────────────
  if (pathname.startsWith('/static/') && WEBUI_RESOURCES) {
    return serveFile(res, path.join(WEBUI_RESOURCES, pathname.slice('/static/'.length)))
  }

  // ── Leo raw design tokens (no browser-theme var() references) ───────────
  if (pathname === '/leo-variables.css') {
    return serveFile(res, LEO_VARIABLES_CSS)
  }
  if (pathname === '/leo-browser.css') {
    return serveFile(res, LEO_BROWSER_CSS)
  }

  // ── Leo SVG icons ────────────────────────────────────────────────────────
  if (pathname.startsWith('/brave-icons/')) {
    return serveFile(res, path.join(LEO_ICONS, pathname.slice('/brave-icons/'.length)))
  }

  // ── Runtime stubs ────────────────────────────────────────────────────────
  if (pathname === '/load_time_data.js') {
    return serveText(res, 'application/javascript; charset=utf-8', LOAD_TIME_DATA_JS)
  }

  if (pathname === '/strings.js') {
    return serveText(res, 'application/javascript; charset=utf-8', STRINGS_JS)
  }

  // ── Webpack bundles / chunks ─────────────────────────────────────────────
  if (pathname.endsWith('.bundle.js') || pathname.endsWith('.chunk.js')) {
    return serveFile(res, path.join(DIST_DIR, path.basename(pathname)))
  }

  // ── Root → main app shell ────────────────────────────────────────────────
  if (pathname === '/' || pathname === '/index.html') {
    return serveFile(res, path.join(__dirname, 'index.html'))
  }

  // ── Fallback: try dist dir ───────────────────────────────────────────────
  const candidate = path.join(DIST_DIR, pathname)
  if (fs.existsSync(candidate) && fs.statSync(candidate).isFile()) {
    return serveFile(res, candidate)
  }

  res.writeHead(404, { 'Content-Type': 'text/plain' })
  res.end(`404 Not Found: ${pathname}`)
})

server.listen(PORT, () => {
  console.log(`AI Chat App  →  http://localhost:${PORT}`)
  console.log(`AI backend   →  ${AI_BACKEND}  (proxied at /v1/)`)
  console.log(`Loaded ${Object.keys(AI_CHAT_STRINGS).length} AI Chat strings`)
  if (!WEBUI_RESOURCES) {
    console.warn(
      'Warning: GN build output not found — CSS/fonts may not load.\n' +
        'Set ROOT_GEN_DIR to a valid gen/ directory.',
    )
  }
})
