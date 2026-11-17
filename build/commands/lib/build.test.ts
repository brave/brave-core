// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { build } from './build.ts'
import * as builderModule from './builder.ts'
import type { Builder } from './builderTypes.ts'
import branding from './branding.js'
import * as buildUtils from './buildUtils.ts'
import { mockConfig } from './configMock.ts'
import util from './util.ts'

const builder: Builder = {
  name: 'fake-builder',
  spec: {
    builder_group: 'brave.test',
    chromium_config: { target_platform: 'linux', build_config: 'Release' },
    gclient_config: { target_os: [], target_cpu: [] },
  },
  buildTarget: { targetOS: 'linux', targetArch: 'x64' },
  siso: {},
  targets: { compile: ['brave:all', 'chrome'], tests: [] },
}

describe('build', () => {
  beforeEach(() => {
    jest.spyOn(buildUtils, 'checkVersionsMatch').mockImplementation(() => {})
    jest.spyOn(buildUtils, 'ensureVsFilesMount').mockImplementation(() => {})
    jest.spyOn(util, 'touchOverriddenFiles').mockImplementation(() => {})
    jest.spyOn(branding, 'update').mockImplementation(() => {})
    jest.spyOn(util, 'generateNinjaFiles').mockResolvedValue(undefined)
    jest.spyOn(util, 'buildTargets').mockResolvedValue(undefined)
    jest.spyOn(builderModule, 'runBotsGen').mockResolvedValue(undefined)
  })

  afterEach(() => {
    jest.restoreAllMocks()
  })

  it('builds the builder compile targets in its own build directory', async () => {
    const config = mockConfig({}, builder)
    await build(undefined, { builder })
    expect(builderModule.runBotsGen).toHaveBeenCalledWith(builder)
    expect(util.generateNinjaFiles).not.toHaveBeenCalled()
    expect(util.buildTargets).toHaveBeenCalledWith(['brave:all', 'chrome'])
    expect(config.outputDir).toMatch(/[/\\]out[/\\]fake-builder$/)
  })

  it('runs gn gen itself without a builder', async () => {
    mockConfig()
    await build('Release', {})
    expect(util.generateNinjaFiles).toHaveBeenCalled()
    expect(builderModule.runBotsGen).not.toHaveBeenCalled()
    expect(util.buildTargets).toHaveBeenCalledWith()
  })
})
