// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

type LoadTimeData = {
  getString: (key: string) => string
  getStringF: (key: string, ...varArgs: string[]) => string
  getInteger: (key: string) => number
  getBoolean: (key: string) => boolean
  set: (value: Record<string, string>) => void
  data_: Record<string, string>
}

export const loadTimeData: LoadTimeData
