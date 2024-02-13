// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/** @type {PluralStringProxyImpl | null} */
let instance = null

export class PluralStringProxyImpl {
  static getInstance() {
    return instance || (instance = new PluralStringProxyImpl())
  }

  /**
   * 
   * @param {string} key 
   * @param {number} count 
   * @returns {Promise<string>}
   */
  getPluralString(key, count) {
    return Promise.resolve(`${key}(${count})`)
  }
}
