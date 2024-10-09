// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const log = new class {
  info(...args: any[]) {
    console.debug('[Rewards]', ...args)
  }

  error(...args: any[]) {
    console.error('[Rewards]', ...args)
  }
}

interface PollOptions {
  name: string
  timeout: number
  interval: number
}

export async function pollFor<T>(fn: () => T | Promise<T>, opts: PollOptions) {
  const startTime = Date.now()
  while (Date.now() - startTime < opts.timeout) {
    const result = await fn()
    if (result) {
      return result
    }
    await new Promise((resolve) => setTimeout(resolve, opts.interval))
  }
  log.info(`Timed out waiting for ${opts.name}`)
  return null
}

export function throttle<T>(fn: () => Promise<T> | T) {
  let current: Promise<T> | null = null
  let next: Promise<T> | null = null

  let start = () => {
    current = Promise.resolve(fn()).finally(() => { current = null })
    return current
  }

  return () => {
    if (!current) {
      return start()
    }
    if (!next) {
      next = current.finally(() => { next = null }).then(start)
    }
    return next
  }
}

export function urlPath(url: string) {
  try { return new URL(url, location.href).pathname }
  catch { return '' }
}

export function absoluteURL(url: string) {
  try { return new URL(url, location.href).toString() }
  catch { return '' }
}

export function getPathComponents(path: string) {
  return path
    .split('/')
    .filter((part) => part)
    .map((part) => part.toLocaleLowerCase())
}
