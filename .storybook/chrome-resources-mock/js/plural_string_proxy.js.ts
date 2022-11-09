// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

let instance: PluralStringProxyImpl | null = null

export class PluralStringProxyImpl {
  static getInstance() {
    return instance || (instance = new PluralStringProxyImpl())
  }

  getPluralString(key: string, count: number): Promise<string> {
    return Promise.resolve(`${key}(${count})`)
  }
}
