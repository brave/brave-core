// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const kScheme = 'chrome-untrusted://'

// chrome-untrusted://view.<uuid>.leo-workspace -> chrome-untrusted://<uuid>.leo-workspace
export function workspaceOrigin(viewerOrigin: string): string {
  const viewPrefix = `${kScheme}view.`
  return viewerOrigin.startsWith(viewPrefix)
    ? viewerOrigin.replace(viewPrefix, kScheme)
    : ''
}

// Parses the #file=<filename> fragment from the URL.
export function parseFileFragment(hash: string): string | null {
  if (!hash || !hash.startsWith('#')) {
    return null
  }
  const params = new URLSearchParams(hash.slice(1))
  return params.get('file')
}

async function waitForServiceWorker(): Promise<void> {
  const registration = await navigator.serviceWorker.ready
  if (!registration.active) {
    throw new Error('No active service worker')
  }
  console.debug('[leo-workspace-view] service worker ready')
}

let contentIframe: HTMLIFrameElement | null = null

function displayFileInIframe(filename: string) {
  const root = document.getElementById('root')
  if (!root) {
    console.error('[leo-workspace-view] #root element not found')
    return
  }

  while (root.firstChild) {
    root.removeChild(root.firstChild)
  }

  if (!contentIframe) {
    contentIframe = document.createElement('iframe')
  }

  contentIframe.src = `/${filename}`
  root.appendChild(contentIframe)

  console.debug('[leo-workspace-view] displaying file:', filename)
}

let serviceWorkerReady = false

async function initialize() {
  console.debug('[leo-workspace-view] bundle loaded at', window.location.origin)

  const wsOrigin = workspaceOrigin(window.location.origin)
  if (!wsOrigin) {
    console.error('[leo-workspace-view] invalid viewer origin')
    return
  }

  const filename = parseFileFragment(window.location.hash)
  if (!filename) {
    window.addEventListener('hashchange', handleHashChange)
    return
  }

  await displayFile(filename)
}

async function handleHashChange() {
  const filename = parseFileFragment(window.location.hash)
  if (filename) {
    await displayFile(filename)
  } else {
    contentIframe = null
  }
}

async function displayFile(filename: string) {
  try {
    if (!serviceWorkerReady) {
      await waitForServiceWorker()
      serviceWorkerReady = true
    }

    displayFileInIframe(filename)
  } catch (error) {
    console.error('[leo-workspace-view] Failed to display file:', error)
  }
}

document.addEventListener('DOMContentLoaded', initialize)
