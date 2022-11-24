// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Based on "classnames" from https://github.com/JedWatson/classnames/tree/5d518653e003a073a7c3846f03bb8821e50b837c
// The MIT License (MIT)
// Copyright (c) 2018 Jed Watson
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

type Value = string | number | boolean | undefined | null | { toString: () => string }
type Mapping = Record<string, unknown>
interface ArgumentArray extends Array<Argument> {}
type Argument = Value | Mapping | ArgumentArray

const hasOwn = {}.hasOwnProperty

/**
 * Converts a list of arguments with truthy values or object with truthy properties to a CSS string.
 */
export default function classnames (...args: Argument[]): string {
  const classes: string[] = []

  for (const arg of args) {
    // Ignore falsey values
    if (!arg) {
      continue
    }
    const argType = typeof arg
    if (argType === 'string' || argType === 'number') {
      classes.push(arg.toString())
    } else if (Array.isArray(arg)) {
      // Flatten the array via recursive call to this function
      if (arg.length) {
        const inner = classnames(...arg)
        if (inner) {
          classes.push(inner)
        }
      }
    } else if (argType === 'object') {
      // Handle an actual object with keys which map to truthy or falsey values
      if (arg.toString === Object.prototype.toString) {
        for (const key in arg as Mapping) {
          // Only take keys if they are directly on the object and have truthy values
          if (hasOwn.call(arg, key) && arg[key]) {
            classes.push(key)
          }
        }
      } else {
        // A different kind of item reporting itself as 'object' type
        classes.push(arg.toString())
      }
    }
  }

  return classes.join(' ')
}
