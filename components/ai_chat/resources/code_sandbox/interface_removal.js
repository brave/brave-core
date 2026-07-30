// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

for (const name of Object.getOwnPropertyNames(globalThis)) {
  if (name.includes('RTC')) {
    globalThis[name] = undefined
  }
}
if (globalThis.DOMParser) {
  globalThis.DOMParser.prototype.parseFromString = undefined
  globalThis.DOMParser = undefined
}
if (globalThis.customElements) {
  globalThis.CustomElementRegistry.prototype.define = undefined
  globalThis.CustomElementRegistry.prototype.get = undefined
  globalThis.customElements = undefined
}

const blockedPrefixes = [
  'get',
  'query',
  'write',
  'append',
  'prepend',
  'import',
  'create',
  'evaluate',
  'replace',
]
for (const target of [document, window.Document.prototype]) {
  for (const name of Object.getOwnPropertyNames(Document.prototype)) {
    if (blockedPrefixes.some((prefix) => name.startsWith(prefix))) {
      target[name] = undefined
    }
  }
}
