// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export interface EnvConfigReader {
  get<T>(keyPath: string[], defaultValue: T): T
  getString(key: string, defaultValue: string): string
  getString(key: string): string | undefined
  pickStrings<K extends string>(...keys: K[]): Record<K, string>
  getPath(key: string): string
  getBoolean(key: string, defaultValue: boolean): boolean
  getBoolean(key: string): boolean | undefined
}
