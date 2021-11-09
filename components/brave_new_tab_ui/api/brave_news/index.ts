// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as BraveNews from 'gen/brave/components/brave_today/common/brave_news.mojom.m.js'
// Provide access to all the generated types
export * from 'gen/brave/components/brave_today/common/brave_news.mojom.m.js'

// Provide easy access to types which mojom functions return but aren't
// defined as a struct.
export type Publishers = Record<string, BraveNews.Publisher>

// Create singleton connection to browser interface
let braveNewsControllerInstance: BraveNews.BraveNewsControllerRemote

export default function getBraveNewsController () {
  // Make connection on first call (not in module root, so that storybook
  // doesn't try to connect, or pages which use exported types
  // but ultimately don't fetch any data.
  if (!braveNewsControllerInstance) {
    braveNewsControllerInstance = BraveNews.BraveNewsController.getRemote()
  }
  return braveNewsControllerInstance
}
