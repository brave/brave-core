// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import config from './config.ts'
import { mockConfig } from './configMock.ts'

describe('config', () => {
  it('throws when read before a config is installed', async () => {
    await jest.isolateModulesAsync(async () => {
      const { default: uninstalled } = await import('./config.ts')
      expect(() => uninstalled.srcDir).toThrow('before a config is installed')
    })
  })

  it('stands for the installed config', () => {
    const installed = mockConfig()
    config.gclientVerbose = true
    expect(installed.gclientVerbose).toBe(true)
    expect(mockConfig().gclientVerbose).toBe(false)
    expect(config.gclientVerbose).toBe(false)
  })

  it('calls methods on the installed config, private fields and all', () => {
    const installed = mockConfig()
    expect(config.getProjectVersion('chrome')).toBe(installed.chromeVersion)
    config.targetOS = 'android'
    expect(config.isAndroid()).toBe(true)
  })

  it('lets jest spy on its methods', () => {
    mockConfig()
    const isAndroid = jest.spyOn(config, 'isAndroid').mockReturnValue(true)
    expect(config.isAndroid()).toBe(true)
    expect(isAndroid).toHaveBeenCalledTimes(1)
  })
})

describe('mockConfig', () => {
  it('takes its .env values from the test', () => {
    expect(mockConfig().gclientVerbose).toBe(false)
    expect(mockConfig({ gclient_verbose: 'true' }).gclientVerbose).toBe(true)
  })
})
