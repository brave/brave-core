// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { transformSync, type PluginItem } from '@babel/core'
import reactCompiler from 'babel-plugin-react-compiler'

/**
 * Runs the React Compiler on the JS output of ts-loader (which has already
 * type-checked the source, stripped types, and converted JSX to
 * `React.createElement`). The compiler inserts automatic memoization
 * (`useMemo` / cache primitives) into components and hooks.
 *
 * Scoped to the opted-in `reactCompilerPaths` in `rules.ts`.
 */
export default function reactCompilerLoader(this: any, source: string, map: any) {
  try {
    const result = transformSync(source, {
      filename: this.resourcePath,
      babelrc: false,
      configFile: false,
      sourceMaps: true,
      inputSourceMap: map,
      plugins: [
        reactCompiler as PluginItem,
      ],
    })

    this.callback(null, result?.code ?? source, result?.map ?? map)
  } catch (err: any) {
    this.callback(new Error(
      `react-compiler-loader error in ${this.resourcePath}:\n${err?.message ?? err}`,
    ))
  }
}
