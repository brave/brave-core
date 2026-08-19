// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { type Command } from 'commander'

// Collects values into an array.
export function collect(
  value: string,
  accumulator: string[] | undefined,
): string[] {
  if (accumulator === undefined) {
    accumulator = []
  }
  accumulator.push(value)
  return accumulator
}

// Use this wrapper function instead of JavaScript's parseInt() with option()
// when defining integer optional parameters, or the default value might get
// passed as well into the radix parameter of parseInt(), causing wrong results.
// https://github.com/brave/brave-browser/issues/13724
export function parseInteger(string: string): number {
  // As per the spec [1], not passing the optional radix parameter to parseInt()
  // will make parsing to interpret the string passed as a decimal number unless
  // it's prefixed with '0' (octal) or '0x' (hexadecimal). We only need decimal
  // in this particular case so let's be explicit about that.
  // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/parseInt
  return parseInt(string, 10)
}

// Parses a boolean value from a string.
export function parseBoolean(value: string): boolean {
  try {
    const parsed = JSON.parse(value)
    if (typeof parsed !== 'boolean') {
      console.error(`Value is not a boolean: ${value}`)
      process.exit(1)
    }
    return parsed
  } catch (error) {
    console.error(`Value is not a boolean: ${value}`)
    process.exit(1)
  }
}

// Returns argv tokens that were not consumed as declared command arguments.
export function getPassthroughArgs(
  command: Command<any[], any, any>,
): string[] {
  return command.args.filter(
    (_, index) => command.processedArgs[index] === undefined,
  )
}
