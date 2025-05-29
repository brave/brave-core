// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
interface Options {
  uppercaseFirstWord?: boolean
  splitWords?: RegExp
}
export function camelCase(str: string, options?: Options) {
  const { uppercaseFirstWord = false, splitWords = /-|_/ } = options || {}
  const result = str.split(splitWords)
    .map((word, i) => (i === 0 && !uppercaseFirstWord)
      ? word.toLowerCase()
      : word[0].toUpperCase() + word.toLowerCase().slice(1))
    .join('')
  console.log(str, splitWords, result)
  return result
}
