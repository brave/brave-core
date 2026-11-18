// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
import type { Builder } from './builderTypes.ts'
import config from './config.ts'
import { mockConfig } from './configMock.ts'
import rootDir from './rootDir.cjs'

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

describe('Config', () => {
  afterEach(() => {
    jest.restoreAllMocks()
  })

  it('reads a property from .env until it is set', () => {
    const installed = mockConfig({ siso_cache_dir: '/siso' })
    expect(installed.sisoCacheDir).toBe(path.normalize('/siso'))
    installed.sisoCacheDir = undefined
    expect(installed.sisoCacheDir).toBeUndefined()
  })

  it('validates target_os from .env as when it is set', () => {
    const exit = jest.spyOn(process, 'exit').mockImplementation(() => {
      throw new Error('exit')
    })
    jest.spyOn(console, 'error').mockImplementation(() => {})
    expect(mockConfig({ target_os: 'android' }).targetOS).toBe('android')
    expect(() => mockConfig({ target_os: 'beos' }).targetOS).toThrow('exit')
    expect(exit).toHaveBeenCalledWith(1)
  })
})

describe('mockConfig', () => {
  it('takes its .env values from the test', () => {
    expect(mockConfig().gclientVerbose).toBe(false)
    expect(mockConfig({ gclient_verbose: 'true' }).gclientVerbose).toBe(true)
  })
})

describe('with --builder', () => {
  const builder: Builder = {
    name: 'fake-builder',
    spec: {
      builder_group: 'brave.test',
      chromium_config: { target_platform: 'android', build_config: 'Release' },
      gclient_config: {
        target_os: ['android'],
        target_cpu: [],
        custom_deps: { 'src/dep': null },
        custom_vars: { checkout_x: true },
      },
      brave_config: {
        is_brave_release_build: false,
        ignore_patch_version_number: false,
        use_brave_hermetic_toolchain: false,
      },
    },
    buildTarget: { targetOS: 'android', targetArch: 'arm64' },
    isBraveOriginBranded: false,
    targets: { compile: [], tests: [] },
  }

  afterEach(() => {
    jest.restoreAllMocks()
  })

  it('reads the allowed .env keys', () => {
    const installed = mockConfig(
      { rbe_service: 'rbe.example.com:443' },
      builder,
    )
    expect(installed.rbeService).toBe('rbe.example.com:443')
  })

  it('takes what it builds from the builder', () => {
    const installed = mockConfig({}, builder)
    expect(installed.targetOS).toBe('android')
    expect(installed.targetArch).toBe('arm64')
    expect(installed.buildConfig).toBe('Release')
    expect(installed.isBraveReleaseBuild()).toBe(false)
  })

  it('takes from the builder what .env otherwise says', () => {
    const releaseBuilder: Builder = {
      ...builder,
      spec: {
        ...builder.spec,
        gclient_config: {
          ...builder.spec.gclient_config,
          global_vars: { a: 1 },
        },
        brave_config: {
          is_brave_release_build: true,
          ignore_patch_version_number: false,
          use_brave_hermetic_toolchain: true,
        },
      },
      isBraveOriginBranded: true,
    }
    const release = mockConfig({}, releaseBuilder)
    expect(release.isBraveReleaseBuild()).toBe(true)
    expect(release.ignorePatchVersionNumber).toBe(false)
    expect(release.useBraveHermeticToolchain).toBe(true)
    expect(release.isBraveOriginBranded).toBe(true)
    expect(release.gclientGlobalVars).toEqual({ a: 1 })

    // Not from rbe_service, whatever service it names.
    const plain = mockConfig({ rbe_service: 'rbe.ba.brave.com:443' }, builder)
    expect(plain.useBraveHermeticToolchain).toBe(false)
    expect(plain.isBraveOriginBranded).toBe(false)
    expect(plain.gclientGlobalVars).toEqual({})
  })

  it('takes none of them when the spec leaves them out', () => {
    const { brave_config: _, ...spec } = builder.spec
    const installed = mockConfig({}, { ...builder, spec })
    expect(installed.isBraveReleaseBuild()).toBe(false)
    expect(installed.ignorePatchVersionNumber).toBe(false)
    expect(installed.useBraveHermeticToolchain).toBe(false)
    expect(installed.gclientGlobalVars).toEqual({})
  })

  it('adds the builder .gclient solution to package.json', () => {
    const installed = mockConfig(
      { rbe_service: 'rbe.example.com:443' },
      builder,
    )
    const chrome = JSON.parse(
      fs.readFileSync(
        path.join(rootDir, 'src', 'brave', 'package.json'),
        'utf8',
      ),
    ).config.projects.chrome
    expect(installed.chromiumCustomDeps).toEqual({
      ...chrome.custom_deps,
      'src/dep': null,
    })
    expect(installed.chromiumCustomVars).toEqual({
      ...chrome.custom_vars,
      checkout_x: true,
      reapi_address: 'rbe.example.com:443',
      reapi_backend_config_path: 'google.star',
      reapi_instance: 'default',
    })
  })

  it('takes the git cache from the environment', () => {
    const saved = process.env.GIT_CACHE_PATH
    process.env.GIT_CACHE_PATH = '/cache/git'
    try {
      expect(mockConfig({}, builder).gitCachePath).toBe('/cache/git')
    } finally {
      if (saved === undefined) {
        delete process.env.GIT_CACHE_PATH
      } else {
        process.env.GIT_CACHE_PATH = saved
      }
    }
  })

  it('leaves anything else in .env unread, the builder secrets included', () => {
    const installed = mockConfig({ brave_services_key: 'secret' }, builder)
    expect(installed.isBuilderSet()).toBe(true)
  })

  it('throws when reading what neither .env nor package.json provides', () => {
    const installed = mockConfig({}, builder)
    expect(() => installed.notary_user).toThrow('notary_user is read')
    expect(() => installed.is_msan).toThrow('is_msan is read')
  })

  it('reads what package.json provides', () => {
    expect(mockConfig({}, builder).chromiumRepo).toBe(
      'https://chromium.googlesource.com/chromium/src.git',
    )
  })

  it('requires the build config in the spec', () => {
    const { build_config: _, ...chromiumConfig } = builder.spec.chromium_config
    expect(() =>
      mockConfig(
        {},
        {
          ...builder,
          spec: { ...builder.spec, chromium_config: chromiumConfig },
        },
      ),
    ).toThrow('builder fake-builder: its spec has no build_config')
  })
})
