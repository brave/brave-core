// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * removes the first character from a Mojo-generated enum key
 * only if the first character is 'k'
 */
export type CleanedFromMojoEnumKey<T extends string | number | symbol> =
  T extends `${infer U}${infer V}`
    ? U extends 'k'
      ? V
      : Exclude<T, 'MIN_VALUE' | 'MAX_VALUE'>
    : never

/** Gets all values from an enum (except min and max value) */
export const getGetMojoEnumValues = <T extends object>(mojoEnum: T) => {
  return (
    Object.keys(mojoEnum)
      // remove MIN/MAX_VALUE props
      .filter((key) => key !== 'MIN_VALUE' && key !== 'MAX_VALUE')
  )
}

export const getGetCleanedMojoEnumKeys = <T extends object>(mojoEnum: T) => {
  return (
    getGetMojoEnumValues(mojoEnum)
      // remove leading 'k' from constant names
      .map((type) =>
        type.startsWith('k') ? type.replace('k', '') : type
      ) as Array<CleanedFromMojoEnumKey<keyof typeof mojoEnum>>
  )
}
