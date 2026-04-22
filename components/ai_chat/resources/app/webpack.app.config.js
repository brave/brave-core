// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Standalone webpack config for the ai_chat_app build.
// Compiles the AI Chat frontend with ai_chat_app=true, pointing at
// a local HTTP/SSE AI backend instead of Chrome Mojo bindings.
// Also emits strings.m.js — a self-contained loadTimeData stub + all AI Chat
// strings parsed from the GRDP source files — so the dev server can serve it
// as a static file without any runtime string generation.

'use strict'

const path = require('path')
const fs = require('fs')
const webpack = require('webpack')
const { fallback, provideNodeGlobals } = require('../../../webpack/polyfill')

const __brave = path.resolve(__dirname, '../../../..')

// Find an existing GN build output that has tsconfig-webpack.json and
// the generated AI Chat mojom JS. Accepts ROOT_GEN_DIR env override.
function findGenDir() {
  const candidates = [
    process.env.ROOT_GEN_DIR,
    path.resolve(__brave, '../out/Component/gen'),
    path.resolve(__brave, '../out/Debug/gen'),
    path.resolve(__brave, '../out/Default/gen'),
    path.resolve(__brave, '../out/Release/gen'),
  ].filter(Boolean)

  for (const d of candidates) {
    if (
      fs.existsSync(path.join(d, 'tsconfig-webpack.json')) &&
      fs.existsSync(path.join(d, 'brave/build_flags.json'))
    ) {
      return d
    }
  }
  throw new Error(
    'No GN build output found. Run a GN build first (e.g. npm run build), ' +
      'or set ROOT_GEN_DIR to the gen/ directory of an existing build.',
  )
}

const ROOT_GEN_DIR = findGenDir()
const DIST_DIR =
  process.env.TARGET_GEN_DIR ||
  path.resolve(__brave, 'dist/ai_chat_app')

const pathMap = require('../../../webpack/path-map')(ROOT_GEN_DIR)

// Load the base build flags from the GN output and override ai_chat_app.
const existingFlags = JSON.parse(
  fs.readFileSync(path.join(ROOT_GEN_DIR, 'brave/build_flags.json'), 'utf8'),
)
const buildFlags = { ...existingFlags, ai_chat_app: true }
const tsConfigPath = path.join(ROOT_GEN_DIR, 'tsconfig-webpack.json')

// Mirrors the prefixReplacer helper from webpack.config.js.
const prefixReplacer = (prefix, replacement) => {
  const regex = new RegExp(`^${prefix}/(.*)`)
  return new webpack.NormalModuleReplacementPlugin(regex, (resource) => {
    resource.request = resource.request.replace(
      regex,
      path.join(replacement, '$1'),
    )
  })
}

// ---------------------------------------------------------------------------
// Static asset generation / copying
// ---------------------------------------------------------------------------

function parseGrdp(filePath) {
  const content = fs.readFileSync(filePath, 'utf8')
  const result = {}
  const msgRe = /<message\s[^>]*name="([^"]+)"[^>]*>([\s\S]*?)<\/message>/g
  let m
  while ((m = msgRe.exec(content)) !== null) {
    const key = m[1].replace(/^IDS_/, '')
    let text = m[2]
    text = text.replace(/<ph[^>]*>([\s\S]*?)<\/ph>/g, '$1')
    text = text.replace(/<[^>]+>/g, '')
    result[key] = text.trim()
  }
  return result
}

// UI feature flags consumed by ai_chat_context.tsx via loadTimeData.
// apiBaseUrl is read by bind_app_conversation.ts for direct-to-backend fetch calls.
const UI_FLAGS = {
  standalone: true,
  isMobile: false,
  isHistoryEnabled: true,
  isAIChatAgentProfileFeatureEnabled: false,
  isAIChatAgentProfile: false,
  apiBaseUrl: process.env.AI_BACKEND || 'http://127.0.0.1:8000',
}

function buildStringsContent(braveDir) {
  const grdpFiles = [
    path.join(braveDir, 'components/resources/ai_chat_ui_strings.grdp'),
    path.join(braveDir, 'components/resources/ai_chat_prompts.grdp'),
  ]
  const strings = Object.assign(
    {},
    ...grdpFiles.filter((f) => fs.existsSync(f)).map(parseGrdp),
  )
  const data = JSON.stringify({ ...UI_FLAGS, ...strings }, null, 2)
  return (
    `window.loadTimeData = {\n` +
    `  data: ${data},\n` +
    `  getString(key) {\n` +
    `    return this.data[key] !== undefined ? String(this.data[key]) : key\n` +
    `  },\n` +
    `  getStringF(key, ...args) {\n` +
    `    let s = this.getString(key)\n` +
    `    args.forEach((arg, i) => { s = s.replaceAll('$' + (i + 1), arg) })\n` +
    `    return s\n` +
    `  },\n` +
    `  getBoolean(key)  { return Boolean(this.data[key]) },\n` +
    `  getInteger(key)  { return parseInt(this.data[key] || '0', 10) },\n` +
    `  getValue(key)    { return this.data[key] },\n` +
    `  valueExists(key) { return key in this.data },\n` +
    `  isInitialized()  { return true },\n` +
    `};\n`
  )
}

// Copies a file/directory into the webpack output directory as build assets.
// Files are emitted as RawSource assets so webpack tracks them properly.
function emitFile(compilation, destName, srcPath) {
  if (!fs.existsSync(srcPath)) return
  const content = fs.readFileSync(srcPath)
  compilation.emitAsset(destName, new webpack.sources.RawSource(content, false))
}

function emitDirRecursive(compilation, destPrefix, srcDir) {
  if (!fs.existsSync(srcDir)) return
  for (const entry of fs.readdirSync(srcDir, { withFileTypes: true })) {
    const dest = `${destPrefix}/${entry.name}`
    const src = path.join(srcDir, entry.name)
    if (entry.isDirectory()) {
      emitDirRecursive(compilation, dest, src)
    } else {
      emitFile(compilation, dest, src)
    }
  }
}

// Rewrites rule for `npx serve dist/ai_chat_app/`.
const SERVE_JSON = JSON.stringify({
  rewrites: [
    { source: '/conversation-entries/:path*', destination: '/conversation-entries.html' },
  ],
}, null, 2)

class StaticAssetsPlugin {
  constructor(options) {
    this.braveDir = options.braveDir
    this.rootGenDir = options.rootGenDir  // may be null
    this.appDir = options.appDir
    this.leoDir = path.join(options.braveDir, 'node_modules/@brave/leo')
  }

  apply(compiler) {
    compiler.hooks.thisCompilation.tap('StaticAssetsPlugin', (compilation) => {
      compilation.hooks.processAssets.tap(
        {
          name: 'StaticAssetsPlugin',
          stage: webpack.Compilation.PROCESS_ASSETS_STAGE_ADDITIONAL,
        },
        () => {
          // strings.m.js — self-contained loadTimeData + all AI Chat strings
          const stringsContent = buildStringsContent(this.braveDir)
          compilation.emitAsset('strings.m.js', new webpack.sources.RawSource(stringsContent))

          // HTML shells
          emitFile(compilation, 'index.html', path.join(this.appDir, 'index.html'))
          emitFile(compilation, 'conversation-entries.html', path.join(this.appDir, 'conversation-entries.html'))

          // Leo design tokens — combine primitives + browser tokens into one file
          {
            const parts = [
              path.join(this.leoDir, 'tokens/css/variables.css'),
              path.join(this.leoDir, 'tokens/css/variables-browser.css'),
            ].filter(fs.existsSync).map((f) => fs.readFileSync(f, 'utf8'))
            compilation.emitAsset(
              'static/css/variables.css',
              new webpack.sources.RawSource(parts.join('\n')),
            )
          }

          // Leo SVG icons (referenced as /brave-icons/... at runtime)
          emitDirRecursive(compilation, 'brave-icons', path.join(this.leoDir, 'icons'))

          // Chrome WebUI CSS + fonts from GN gen dir (optional — degrades gracefully)
          if (this.rootGenDir) {
            const webui = path.join(this.rootGenDir, 'brave/ui/webui/resources')
            emitFile(compilation, 'static/css/reset.css', path.join(webui, 'css/reset.css'))
            emitDirRecursive(compilation, 'static/fonts', path.join(webui, 'fonts'))
          }

          // serve.json — URL rewrites for `npx serve dist/ai_chat_app/`
          compilation.emitAsset('serve.json', new webpack.sources.RawSource(SERVE_JSON))
        },
      )
    })
  }
}

// ---------------------------------------------------------------------------
// Model list generation from model_service.cc
// ---------------------------------------------------------------------------

function generateModelsTs(braveDir) {
  const modelServiceFile = path.join(
    braveDir, 'components/ai_chat/core/browser/model_service.cc',
  )
  const constantsFiles = [
    path.join(braveDir, 'components/ai_chat/core/common/constants.h'),
    path.join(braveDir, 'components/ai_chat/core/browser/constants.h'),
    modelServiceFile,
  ]

  const source = fs.readFileSync(modelServiceFile, 'utf8')

  // Collect all string constants from headers + model_service.cc
  const consts = {}
  for (const f of constantsFiles) {
    if (!fs.existsSync(f)) continue
    const c = fs.readFileSync(f, 'utf8')
    const re = /constexpr\s+char\s+(\w+)\[\]\s*=\s*"([^"]*)"/g
    let m
    while ((m = re.exec(c)) !== null) consts[m[1]] = m[2]
  }

  function resolveStr(val) {
    val = val.trim()
    const quoted = val.match(/^"(.*)"$/)
    if (quoted) return quoted[1]
    return consts[val] ?? val
  }

  // For each occurrence of `auto model = mojom::Model::New()`, extract the
  // enclosing anonymous { … } block and parse it into a model object.
  const models = []
  const marker = 'auto model = mojom::Model::New()'
  let searchPos = 0

  while (true) {
    const idx = source.indexOf(marker, searchPos)
    if (idx === -1) break
    searchPos = idx + marker.length

    // Walk backward to the opening brace of the enclosing block.
    let depth = 0
    let blockStart = -1
    for (let i = idx; i >= 0; i--) {
      if (source[i] === '}') depth++
      else if (source[i] === '{') {
        if (depth === 0) { blockStart = i; break }
        depth--
      }
    }
    if (blockStart < 0) continue

    // Walk forward to the matching closing brace.
    depth = 0
    let blockEnd = -1
    for (let i = blockStart; i < source.length; i++) {
      if (source[i] === '{') depth++
      else if (source[i] === '}') {
        if (--depth === 0) { blockEnd = i; break }
      }
    }
    if (blockEnd < 0) continue

    const block = source.slice(blockStart, blockEnd + 1)

    // Skip blocks inside `if (features::…)` guards.
    const pre = source.slice(Math.max(0, blockStart - 200), blockStart)
    if (/if\s*\(\s*features::/.test(pre)) continue

    function field(re, def = '') {
      const m = block.match(re)
      return m ? resolveStr(m[1]) : def
    }

    const key         = field(/model->key\s*=\s*([^;]+);/)
    const displayName = field(/model->display_name\s*=\s*([^;]+);/)
    const name        = field(/options->name\s*=\s*([^;]+);/)
    const displayMaker = field(/options->display_maker\s*=\s*([^;]+);/, '')

    if (!key || !displayName || !name) continue

    const visionSupport   = /model->vision_support\s*=\s*true/.test(block)
    const isSuggestedModel = /model->is_suggested_model\s*=\s*true/.test(block)
    const isNearModel     = /model->is_near_model\s*=\s*true/.test(block)

    // supports_tools may be a feature flag expression — default to true.
    const toolsMatch = block.match(/model->supports_tools\s*=\s*([^;]+);/)
    const supportsTools = toolsMatch
      ? (toolsMatch[1].trim() === 'true' ? true : toolsMatch[1].trim() === 'false' ? false : true)
      : false

    // Access: only PREMIUM when ::PREMIUM appears with no BASIC in the expression.
    const accessMatch = block.match(/options->access\s*=\s*([^;]+);/)
    const access = (() => {
      if (!accessMatch) return 'BASIC_AND_PREMIUM'
      const raw = accessMatch[1]
      return raw.includes('::PREMIUM') && !raw.includes('BASIC') ? 'PREMIUM' : 'BASIC_AND_PREMIUM'
    })()

    const maxContent = (() => {
      const m = block.match(/max_associated_content_length\s*=\s*(\d+)/)
      return m ? parseInt(m[1]) : 64000
    })()
    const warnLimit = (() => {
      const m = block.match(/long_conversation_warning_character_limit\s*=\s*(\d+)/)
      return m ? parseInt(m[1]) : 9700
    })()

    // Gather all ConversationCapability values mentioned in the block
    // (handles both simple arrays and ternary expressions).
    const caps = []
    const capRe = /ConversationCapability::(\w+)/g
    let cm
    while ((cm = capRe.exec(block)) !== null) {
      if (!caps.includes(cm[1])) caps.push(cm[1])
    }

    models.push({
      key, displayName, name, displayMaker, access,
      visionSupport, supportsTools, caps,
      maxContent, warnLimit, isSuggestedModel, isNearModel,
    })
  }

  // Emit TypeScript source.
  const lines = [
    '// Copyright (c) 2026 The Brave Authors. All rights reserved.',
    '// This Source Code Form is subject to the terms of the Mozilla Public',
    '// License, v. 2.0. If a copy of the MPL was not distributed with this file,',
    '// You can obtain one at https://mozilla.org/MPL/2.0/.',
    '',
    '// @generated — do not edit manually.',
    '// Regenerated from model_service.cc on every `npm run ai-chat-app:build`.',
    '',
    "import * as Mojom from '../../../common/mojom'",
    '',
    'export const APP_MODELS: Mojom.Model[] = [',
  ]

  for (const m of models) {
    const caps = m.caps.map((c) => `Mojom.ConversationCapability.${c}`).join(', ')
    lines.push(`  {`)
    lines.push(`    key: ${JSON.stringify(m.key)},`)
    lines.push(`    displayName: ${JSON.stringify(m.displayName)},`)
    lines.push(`    visionSupport: ${m.visionSupport},`)
    lines.push(`    supportsTools: ${m.supportsTools},`)
    lines.push(`    audioSupport: false,`)
    lines.push(`    videoSupport: false,`)
    lines.push(`    supportedCapabilities: [${caps}],`)
    lines.push(`    isSuggestedModel: ${m.isSuggestedModel},`)
    lines.push(`    isNearModel: ${m.isNearModel},`)
    lines.push(`    options: {`)
    lines.push(`      leoModelOptions: {`)
    lines.push(`        name: ${JSON.stringify(m.name)},`)
    lines.push(`        displayMaker: ${JSON.stringify(m.displayMaker)},`)
    lines.push(`        description: '',`)
    lines.push(`        category: Mojom.ModelCategory.CHAT,`)
    lines.push(`        access: Mojom.ModelAccess.${m.access},`)
    lines.push(`        maxAssociatedContentLength: ${m.maxContent},`)
    lines.push(`        longConversationWarningCharacterLimit: ${m.warnLimit},`)
    lines.push(`      },`)
    lines.push(`    } as Mojom.ModelOptions,`)
    lines.push(`  },`)
  }

  lines.push(']', '', "export const DEFAULT_MODEL_KEY = 'chat-automatic'", '')
  return lines.join('\n')
}

class GenerateModelsPlugin {
  constructor(braveDir, sourceDir) {
    this.braveDir = braveDir
    this.sourceDir = sourceDir
  }

  apply(compiler) {
    compiler.hooks.beforeCompile.tapAsync('GenerateModelsPlugin', (_params, callback) => {
      try {
        const content = generateModelsTs(this.braveDir)
        const outPath = path.join(this.sourceDir, 'app_models_generated.ts')
        fs.writeFileSync(outPath, content, 'utf8')
      } catch (err) {
        return callback(err)
      }
      callback()
    })
  }
}

// ---------------------------------------------------------------------------

module.exports = async function (_env, argv) {
  const isDevMode = (argv.mode || 'development') !== 'production'

  return {
    entry: {
      chat_ui: path.join(
        __brave,
        'components/ai_chat/resources/page/chat_ui.tsx',
      ),
      untrusted_conversation_frame: path.join(
        __brave,
        'components/ai_chat/resources/untrusted_conversation_frame/untrusted_conversation_frame.tsx',
      ),
    },
    devtool: isDevMode ? 'inline-source-map' : false,
    output: {
      iife: true,
      path: DIST_DIR,
      filename: '[name].bundle.js',
      chunkFilename: '[name].chunk.js',
      publicPath: '/',
    },
    resolve: {
      extensions: ['.js', '.tsx', '.ts', '.json'],
      symlinks: false,
      alias: pathMap,
      modules: ['node_modules'],
      fallback,
    },
    optimization: {
      chunkIds: false,
      moduleIds: false,
    },
    experiments: {
      asyncWebAssembly: true,
    },
    plugins: [
      new webpack.ids.NamedModuleIdsPlugin({ context: ROOT_GEN_DIR }),
      new webpack.ids.NamedChunkIdsPlugin({ context: ROOT_GEN_DIR }),
      provideNodeGlobals,
      new StaticAssetsPlugin({
        braveDir: __brave,
        rootGenDir: ROOT_GEN_DIR,
        appDir: __dirname,
      }),
      new GenerateModelsPlugin(
        __brave,
        path.join(__brave, 'components/ai_chat/resources/page/api/app'),
      ),
      ...Object.keys(pathMap)
        .filter((p) => p.startsWith('chrome://'))
        .map((p) => prefixReplacer(p, pathMap[p])),
    ],
    module: {
      rules: [
        {
          test: /\.s?css$/,
          include: [/\.global\./, /node_modules/],
          use: [{ loader: 'style-loader' }, { loader: 'css-loader' }],
        },
        {
          test: /\.s?css$/,
          exclude: [/\.global\./, /node_modules/],
          use: [
            { loader: 'style-loader' },
            {
              loader: 'css-loader',
              options: {
                importLoaders: 3,
                sourceMap: false,
                modules: {
                  localIdentName: isDevMode
                    ? '[path][name]__[local]--[contenthash:base64:5]'
                    : '[contenthash:base64]',
                },
              },
            },
            { loader: 'sass-loader' },
          ],
        },
        {
          test: /\.tsx?$/,
          loader: 'ts-loader',
          options: {
            getCustomTransformers: path.join(
              __brave,
              'components/webpack/webpack-ts-transformers.js',
            ),
            configFile: tsConfigPath,
          },
        },
        {
          test: /\.(js|ts)x?$/,
          loader: path.join(
            __brave,
            'components/webpack/plugins/ifdef-loader.ts',
          ),
          options: buildFlags,
        },
        {
          test: /\.(ttf|eot|ico|svg|png|jpg|jpeg|gif|webp)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
          loader: 'file-loader',
        },
        {
          test: /\.html/,
          type: 'asset/source',
        },
        {
          test: (p) =>
            p.includes(path.join('@brave', 'brave-ui')) && p.endsWith('.js'),
          resolve: { fullySpecified: false },
        },
      ],
    },
  }
}
