// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import path from 'path'
import type { RuleSetRule } from 'webpack'
import getTsCustomTransformers from './webpack-ts-transformers.js'
import dirName from './dirName.cjs'

// Resolved against this directory so the rules work regardless of which config
// imports them.
const ifdefLoaderPath = path.join(dirName, 'plugins', 'ifdef-loader.ts')

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
}: {
  isDevMode: boolean
  test?: RegExp
}): RuleSetRule[] {
  return [
    {
      // CSS imported from node_modules or in a x.global.css file is just regular
      // css converted to JS and injected to style elements.
      test,
      include: [/\.global\./, /node_modules/],
      use: [{ loader: 'style-loader' }, { loader: 'css-loader' }],
    },
    {
      // CSS imported in the source tree can use sass and css modules syntax.
      test,
      exclude: [/\.global\./, /node_modules/],
      use: [
        // Injects the result into the DOM as a style block.
        { loader: 'style-loader' },
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
