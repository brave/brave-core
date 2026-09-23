// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'node:path'
import type { RuleSetRule } from 'webpack'
import getTsCustomTransformers from './webpack-ts-transformers.js'
import dirName from './dirName.cjs'

// Resolved against this directory so the rules work regardless of which config
// imports them.
const ifdefLoaderPath = path.join(dirName, 'plugins', 'ifdef-loader.ts')

/**
 * KaTeX's stylesheet declares each of its 20 fonts with a `.woff2` source
 * followed by `.woff` and `.ttf` fallbacks. Chromium has supported woff2 since
 * M36, so those fallbacks would add ~876KB of resources that are never
 * requested — a font `src` list only advances past an entry that fails to load,
 * and woff2 is listed first.
 *
 * Filtered `url()`s are left in the emitted CSS exactly as authored rather than
 * resolved to a bundled asset, so the files stay out of the build entirely.
 */
const katexCss = /katex[\\/]dist[\\/]katex\.css$/
const isLegacyFontUrl = (url: string) => /\.(?:woff|ttf)$/i.test(url)

function katexCssRule({
  styleLoaderOptions = {},
}: {
  styleLoaderOptions?: Record<string, any>
} = {}): RuleSetRule {
  return {
    test: katexCss,
    use: [
      { loader: 'style-loader', options: styleLoaderOptions },
      {
        loader: 'css-loader',
        // css-loader 5 takes `url` as the filter predicate itself; the
        // `{ filter }` object form is only available from v6.
        options: { url: (url: string) => !isLegacyFontUrl(url) },
      },
    ],
  }
}

/**
 * Loaders for SASS / CSS. `.global.` files and anything from `node_modules` are
 * injected verbatim as a style block; source-tree styles additionally go
 * through CSS modules (class names are renamed unless wrapped in `:global(...)`).
 *
 * @param opts.isDevMode Whether to emit readable (vs. hashed) class names.
 * @param opts.test The file matcher. Defaults to `.css` and `.scss`; Storybook
 *   narrows this to `.scss` to avoid clobbering its built-in `.css` handling.
 */
export function cssRules({
  isDevMode,
  test = /\.s?css$/,
  styleLoaderOptions = {},
}: {
  isDevMode: boolean
  test?: RegExp
  styleLoaderOptions?: Record<string, any>
}): RuleSetRule[] {
  // Whether this caller's matcher covers plain `.css`, and so owns KaTeX's
  // stylesheet. Bundling the rule in here rather than making each config add it
  // separately means a new config cannot end up with katex.css matching no rule
  // at all, and means the rule inherits the same `styleLoaderOptions` as
  // everything else — which matters for bundles that inject into a shadow root,
  // since styles left in `document.head` would not reach the KaTeX markup.
  // Storybook narrows `test` to `.scss` so that its built-in `.css` handling
  // stays in charge, and must not get this rule.
  const ownsPlainCss = test.test('example.css')

  return [
    ...(ownsPlainCss ? [katexCssRule({ styleLoaderOptions })] : []),
    {
      // CSS imported from node_modules or in a x.global.css file is just regular
      // css converted to JS and injected to style elements.
      test,
      include: [/\.global\./, /node_modules/],
      // Handled by katexCssRule above, which drops its dead font fallbacks.
      ...(ownsPlainCss ? { exclude: [katexCss] } : {}),
      use: [{ loader: 'style-loader' }, { loader: 'css-loader' }],
    },
    {
      // CSS imported in the source tree can use sass and css modules syntax.
      test,
      exclude: [/\.global\./, /node_modules/],
      use: [
        // Injects the result into the DOM as a style block.
        { loader: 'style-loader', options: styleLoaderOptions },
        // Converts the resulting CSS to Javascript to be bundled (modules:true to
        // rename CSS classes in output to cryptic identifiers, except if wrapped
        // in a :global(...) pseudo class).
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
        // First, convert SASS to CSS.
        { loader: 'sass-loader' },
      ],
    },
  ]
}

/**
 * ts-loader with a styled-components transformer.
 *
 * @param opts.configFile The generated tsconfig pointing at the build's gen dir.
 * @param opts.transpileOnly Skip type-checking (e.g. when a separate
 *   ForkTsChecker pass handles errors).
 */
export function tsLoaderRule({
  configFile,
  transpileOnly = false,
}: {
  configFile: string
  transpileOnly?: boolean
}): RuleSetRule {
  return {
    test: /\.tsx?$/,
    loader: 'ts-loader',
    options: {
      transpileOnly,
      getCustomTransformers: getTsCustomTransformers,
      // Use generated tsconfig so that we can point at gen/ output in the correct
      // build configuration output directory.
      configFile,
    },
  }
}

/**
 * Strips code behind disabled build flags (`#if`/`#endif` style comments).
 *
 * @param buildFlags The parsed contents of brave/build_flags.json.
 */
export function ifdefLoaderRule(buildFlags: {
  [key: string]: boolean
}): RuleSetRule {
  return {
    test: /\.(js|ts)x?$/,
    loader: ifdefLoaderPath,
    options: buildFlags,
  }
}

/**
 * file-loader for binary assets. Pass extra extensions a particular build needs
 * (e.g. `['woff2']`).
 */
export function fileLoaderRule(): RuleSetRule {
  return {
    test: new RegExp(
      `\\.(${['ttf', 'woff2', 'eot', 'ico', 'svg', 'png', 'jpg', 'jpeg', 'gif', 'webp'].join('|')})(\\?v=[0-9]\\.[0-9]\\.[0-9])?$`,
    ),
    loader: 'file-loader',
  }
}

/**
 * Emits onnxruntime-web's worker glue script as a served asset instead of
 * inlining it into the bundle. A Worker downloads its own script from a URL, so
 * it must be served separately from the bundle. `dependency: 'url'` scopes this
 * rule to ORT's own `new URL()` reference and leaves our own import of the same
 * file alone. The `.mjs` is renamed to `.js` so it serves as
 * application/javascript.
 *
 * TODO(https://github.com/brave/brave-browser/issues/57166): this rule is
 * specific to a dependency (onnxruntime-web). Figure out how to make it more
 * generic in the future.
 */
export function onnxRuntimeWorkerJsRule(): RuleSetRule {
  return {
    test: /\.mjs$/,
    include: /onnxruntime-web/,
    dependency: 'url',
    type: 'asset/resource',
    generator: { filename: '[name].[contenthash].js' },
  }
}

/**
 * Emits onnxruntime-web's `.wasm` as a served asset so the import resolves to a
 * URL rather than an instantiated module. Scoped to onnxruntime-web by path: a
 * blanket `.wasm` rule would also catch wasm-bindgen crates (e.g. opaque_ke)
 * that import their `.wasm` as a module and rely on its named exports.
 */
export function onnxRuntimeWasmRule(): RuleSetRule {
  return {
    test: /\.wasm$/,
    include: /onnxruntime-web/,
    type: 'asset/resource',
    generator: { filename: '[name].[contenthash][ext]' },
  }
}

// brave-ui is compiled as a "module" so Webpack5 expects it to provide file
// extensions (which it does not), so we need to special case it here.
export const braveUiFullySpecifiedRule: RuleSetRule = {
  test: (p: string) =>
    p.includes(path.join('@brave', 'brave-ui')) && p.endsWith('.js'),
  resolve: {
    fullySpecified: false,
  },
}

export const htmlAssetRule: RuleSetRule = {
  test: /\.html/,
  type: 'asset/source',
}
