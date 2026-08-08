// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Argument } from 'commander'
import * as Log from './log.ts'

// Returns an Argument that parses the current build configuration
export function createBuildConfigArgument() {
  // Build config argument that's valid only if it's not an option.
  return new Argument('[build_config]', 'build configuration').argParser(
    (value) => {
      if (value.startsWith('-')) {
        return undefined
      }
      return value
    },
  )
}

// Collects the value into the accumulator.
export function collect(value: string, accumulator: string[]): string[] {
  accumulator.push(value)
  return accumulator
}

export function argParserBoolean(value: string): boolean {
  try {
    const parsed = JSON.parse(value)
    if (typeof parsed !== 'boolean') {
      throw new Error(`Invalid boolean value: ${value}`)
    }
    return parsed
  } catch (error) {
    Log.error(`Invalid boolean value: ${value}`)
    process.exit(1)
  }
}
