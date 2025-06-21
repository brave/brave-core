// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ExtensionsItemElement } from './item-chromium.js'

import { loadTimeData } from '//resources/js/load_time_data.js'

declare module './item-chromium.js' {
  interface ExtensionsItemElement {
    isBraveHosted_: (extensionId: string) => boolean
  }
}

ExtensionsItemElement.prototype.isBraveHosted_ = (extensionId: string) => {
  return loadTimeData.getString('braveHostedExtensions').includes(extensionId)
}

export * from './item-chromium.js'
