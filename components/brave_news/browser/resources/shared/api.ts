// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as BraveNews from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'
// Provide access to all the generated types
export * from 'gen/brave/components/brave_news/common/brave_news.mojom.m.js'

// Provide easy access to types which mojom functions return but aren't
// defined as a struct.
export type Publishers = Record<string, BraveNews.Publisher>
export type Channels = Record<string, BraveNews.Channel>

declare global {
  interface Window {
    // In Storybook there's no Mojo backend, so a mock controller is registered
    // here for getBraveNewsController() to return instead of a real connection.
    storybookBraveNewsController?: BraveNews.BraveNewsControllerRemote
  }
}

// Create singleton connection to browser interface
let braveNewsControllerInstance: BraveNews.BraveNewsControllerRemote

export default function getBraveNewsController () {
  // Make connection on first call (not in module root, so that storybook
  // doesn't try to connect, or pages which use exported types
  // but ultimately don't fetch any data.
  if (!braveNewsControllerInstance) {
    // In Storybook, we have a mocked BraveNewsController because none of the
    // mojo apis are available.
    braveNewsControllerInstance = window.storybookBraveNewsController || BraveNews.BraveNewsController.getRemote()
  }
  return braveNewsControllerInstance
}

// The listener wrappers (configuration/channels/publishers/feed) bind a Mojo
// pipe to receive updates pushed from the browser. That isn't possible without
// a real backend, which is absent both in unit tests and in Storybook (where a
// mock controller stands in). Callers use this to skip binding in those cases.
export function canBindMojoListeners () {
  return process.env.NODE_ENV !== 'test' && !window.storybookBraveNewsController
}

export const isPublisherEnabled = (publisher: BraveNews.Publisher) => {
  if (!publisher) return false

  // Direct Sources are enabled if they're available.
  if (publisher.type === BraveNews.PublisherType.DIRECT_SOURCE) return true

  // Publishers enabled via channel are not shown in the sidebar.
  return publisher.userEnabledStatus === BraveNews.UserEnabled.ENABLED
}

export const isDirectFeed = (publisher: BraveNews.Publisher) => {
  if (!publisher) return false
  return publisher.type === BraveNews.PublisherType.DIRECT_SOURCE
}
