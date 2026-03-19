// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import assert from 'node:assert'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import EnvConfig from './envConfig.ts'
import * as Log from './log.ts'

/* eslint jest/expect-expect: ["error", { "assertFunctionNames": ["expect*"] }] */

// Mock the logging module
jest.mock('./log.ts', () => ({
  error: jest.fn(),
}))

describe('EnvConfig', () => {
  const configDir = '/path/to/config'
  const packageJsonPath = path.join(configDir, 'package.json')
  const envPath = path.join(configDir, '.env')

  let mockFiles = {}
  let envConfig: EnvConfig

  const expectInvalidTypeError = (fn: () => unknown) => {
    expect(fn).toThrow('process.exit called')
    expect(Log.error).toHaveBeenCalledWith(
      expect.stringContaining('value type is invalid'),
    )
  }

  const expectRequiredNotFound = (fn: () => unknown) => {
    expect(fn).toThrow('process.exit called')
    expect(Log.error).toHaveBeenCalledWith(
      expect.stringMatching(/Required config value .* is not set/),
    )
  }

  beforeEach(() => {
    jest.clearAllMocks()
    mockFiles = {}

    // Mock fs.existsSync
    jest.spyOn(fs, 'existsSync').mockImplementation((filePath) => {
      return mockFiles.hasOwnProperty(filePath.toString())
    })

    // Mock fs.readFileSync
    jest.spyOn(fs, 'readFileSync').mockImplementation((filePath) => {
      if (!mockFiles.hasOwnProperty(filePath.toString())) {
        throw new Error(`ENOENT: no such file or directory, open '${filePath}'`)
      }
      const f = mockFiles[filePath.toString()]
      if (typeof f === 'string') {
        return f
      }
      if (Array.isArray(f)) {
        return f.join('\n')
      }
      if (typeof f === 'object') {
        return JSON.stringify(f)
      }
      throw new Error(`Unsupported file type: ${typeof f}`)
    })

    // Mock fs.writeFileSync
    jest.spyOn(fs, 'writeFileSync').mockImplementation((filePath, data) => {
      mockFiles[filePath.toString()] = data
    })

    // Mock process.exit to prevent tests from exiting
    jest.spyOn(process, 'exit').mockImplementation(() => {
      throw new Error('process.exit called')
    })
  })

  afterEach(() => {
    jest.restoreAllMocks()
  })

  describe('constructor', () => {
    it('should load package.json and .env on construction', () => {
      mockFiles[packageJsonPath] = {
        version: '1.2.3',
        config: {
          projects: {
            chrome: {
              version: '120.0.0',
            },
          },
        },
      }
      mockFiles[envPath] = 'TEST_VALUE=hello'

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.getPackageVersion()).toBe('1.2.3')
      expect(envConfig.getString(['projects', 'chrome', 'version'])).toBe(
        '120.0.0',
      )
      expect(envConfig.getString(['TEST', 'VALUE'])).toBe('hello')
    })

    it('should create .env file if it does not exist', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }

      const envConfig = new EnvConfig(configDir)

      expect(envConfig).toBeDefined()
      expect(fs.writeFileSync).toHaveBeenCalledWith(
        envPath,
        expect.stringContaining('placeholder .env config file'),
      )
    })
  })

  function runCommonConfigTests() {
    it('should retrieve version', () => {
      expect(envConfig.getPackageVersion()).toBe('1.2.3')
    })

    it('should retrieve config values', () => {
      expect(envConfig.getBoolean(['boolean'])).toBe(true)
      expect(envConfig.getNumber(['number'])).toBe(42)
      expect(envConfig.getString(['string'])).toBe('hello')
      expect(envConfig.getArray(['array'])).toEqual(['a', 'b', 'c'])
      expect(envConfig.getObject(['object'])).toEqual({ key: 'value' })
      expect(envConfig.getString(['object', 'key'])).toBe('value')

      expect(envConfig.requireBoolean(['boolean'])).toBe(true)
      expect(envConfig.requireNumber(['number'])).toBe(42)
      expect(envConfig.requireString(['string'])).toBe('hello')
      expect(envConfig.requireArray(['array'])).toEqual(['a', 'b', 'c'])
      expect(envConfig.requireObject(['object'])).toEqual({ key: 'value' })
      expect(envConfig.requireString(['object', 'key'])).toBe('value')

      expect(envConfig.getAny(['boolean'])).toBe(true)
      expect(envConfig.getAny(['number'])).toBe(42)
      expect(envConfig.getAny(['string'])).toBe('hello')
      expect(envConfig.getAny(['array'])).toEqual(['a', 'b', 'c'])
      expect(envConfig.getAny(['object'])).toEqual({ key: 'value' })
      expect(envConfig.getAny(['object', 'key'])).toBe('value')
    })

    it('non-existent config values', () => {
      expect(envConfig.getString(['a'])).toBeUndefined()
      expect(envConfig.getString(['a', 'b'])).toBeUndefined()
      expect(envConfig.getString(['object', 'b'])).toBeUndefined()

      expectRequiredNotFound(() => envConfig.requireString(['a']))
      expectRequiredNotFound(() => envConfig.requireString(['a', 'b']))
      expectRequiredNotFound(() => envConfig.requireString(['object', 'b']))

      expect(envConfig.getAny(['a'])).toBeUndefined()
      expect(envConfig.getAny(['a', 'b'])).toBeUndefined()
      expect(envConfig.getAny(['object', 'b'])).toBeUndefined()
    })

    it('default config values', () => {
      expect(envConfig.getBoolean(['a'], true)).toBe(true)
      expect(envConfig.getNumber(['a'], 0)).toBe(0)
      expect(envConfig.getString(['a'], 'default')).toBe('default')
      expect(envConfig.getArray(['a'], ['default'])).toEqual(['default'])
      expect(envConfig.getObject(['a'], { key: 'default' })).toEqual({
        key: 'default',
      })
    })

    it('should throw if type does not match', () => {
      expectInvalidTypeError(() => envConfig.getBoolean(['number']))
      expectInvalidTypeError(() => envConfig.getNumber(['boolean']))
      expectInvalidTypeError(() => envConfig.getArray(['boolean']))
      expectInvalidTypeError(() => envConfig.getObject(['boolean']))

      expectInvalidTypeError(() => envConfig.requireBoolean(['number']))
      expectInvalidTypeError(() => envConfig.requireNumber(['boolean']))
      expectInvalidTypeError(() => envConfig.requireArray(['boolean']))
      expectInvalidTypeError(() => envConfig.requireObject(['boolean']))
    })

    it('should return the same value for the same key', () => {
      const result1 = envConfig.getObject(['object'])
      const result2 = envConfig.getObject(['object'])

      expect(result1).toEqual(result2)
    })
  }

  describe('without .env', () => {
    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.2.3',
        config: {
          boolean: true,
          number: 42,
          string: 'hello',
          array: ['a', 'b', 'c'],
          object: {
            key: 'value',
          },
        },
      }

      envConfig = new EnvConfig(configDir)
    })

    runCommonConfigTests()

    it('getString/requireString should throw if type does not match', () => {
      expectInvalidTypeError(() => envConfig.getString(['boolean']))
      expectInvalidTypeError(() => envConfig.requireString(['boolean']))
    })
  })

  describe('with .env', () => {
    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.2.3',
        config: {
          fallback_value: 'from_package',
        },
      }
      mockFiles[envPath] = [
        'version=4.5.6',
        'boolean=true',
        'number=42',
        'string=hello',
        'array=["a","b","c"]',
        'object={"key":"value"}',
        'object_key=value',
      ]

      envConfig = new EnvConfig(configDir)
    })

    runCommonConfigTests()

    it('getString/requireString should always return string values', () => {
      expect(envConfig.getString(['boolean'])).toBe('true')
      expect(envConfig.getString(['number'])).toBe('42')
      expect(envConfig.getString(['string'])).toBe('hello')
      expect(envConfig.getString(['array'])).toEqual('["a","b","c"]')
      expect(envConfig.getString(['object'])).toEqual('{"key":"value"}')
      expect(envConfig.getString(['object', 'key'])).toBe('value')

      expect(envConfig.requireString(['boolean'])).toBe('true')
      expect(envConfig.requireString(['number'])).toBe('42')
      expect(envConfig.requireString(['string'])).toBe('hello')
      expect(envConfig.requireString(['array'])).toEqual('["a","b","c"]')
      expect(envConfig.requireString(['object'])).toEqual('{"key":"value"}')
      expect(envConfig.requireString(['object', 'key'])).toBe('value')
    })

    it('should fall back to package config when .env value not found', () => {
      expect(envConfig.getString(['fallback_value'])).toBe('from_package')
    })
  })

  describe('getMergedObject', () => {
    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.2.3',
        config: {
          projects: {
            chrome: {
              custom_deps: {
                'src/third_party/chromium-variations': null,
              },
              custom_vars: {
                'checkout_clangd': true,
              },
            },
          },
        },
      }
      envConfig = new EnvConfig(configDir)
    })

    it('should return package.json values when .env is not present', () => {
      expect(
        envConfig.getMergedObject(['projects', 'chrome', 'custom_deps']),
      ).toEqual({
        'src/third_party/chromium-variations': null,
      })

      expect(
        envConfig.getMergedObject(['projects', 'chrome', 'custom_vars']),
      ).toEqual({
        'checkout_clangd': true,
      })
    })

    it('should merge .env and package.json values', () => {
      mockFiles[envPath] = [
        'projects_chrome_custom_deps={"src/third_party/chromium-variations": "rev"}',
        'projects_chrome_custom_vars={"checkout_clang_tidy": true}',
      ]
      envConfig = new EnvConfig(configDir)

      expect(
        envConfig.getMergedObject(['projects', 'chrome', 'custom_deps']),
      ).toEqual({
        'src/third_party/chromium-variations': 'rev',
      })
      expect(
        envConfig.getMergedObject(['projects', 'chrome', 'custom_vars']),
      ).toEqual({
        'checkout_clangd': true,
        'checkout_clang_tidy': true,
      })
    })

    it('should merge .env and package.json values with key prefix values', () => {
      mockFiles[envPath] = [
        'projects_chrome_custom_vars={"checkout_clang_tidy": false}',
        'projects_chrome_custom_vars_checkout_clang_tidy=true',
      ]
      envConfig = new EnvConfig(configDir)

      expect(
        envConfig.getMergedObject(['projects', 'chrome', 'custom_vars']),
      ).toEqual({
        'checkout_clangd': true,
        'checkout_clang_tidy': true,
      })
    })
  })

  describe('include_env directive', () => {
    it('should handle include_env directive in .env files', () => {
      const includedEnvPath = path.resolve(configDir, 'included.env')

      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = [
        'MAIN_VALUE=main',
        'include_env=included.env',
        'OVERRIDE_VALUE=from_main',
      ]
      mockFiles[includedEnvPath] = [
        'INCLUDED_VALUE=included',
        'OVERRIDE_VALUE=from_included',
      ]

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.getString(['MAIN', 'VALUE'])).toBe('main')
      expect(envConfig.getString(['INCLUDED', 'VALUE'])).toBe('included')
      expect(envConfig.getString(['OVERRIDE', 'VALUE'])).toBe('from_main')
    })

    it('should handle nested include_env directives', () => {
      const included1Path = path.resolve(configDir, 'included1.env')
      const included2Path = path.resolve(configDir, 'included2.env')

      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = ['MAIN=1', 'include_env=included1.env']
      mockFiles[included1Path] = ['LEVEL1=2', 'include_env=included2.env']
      mockFiles[included2Path] = `LEVEL2=3`

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.getNumber(['MAIN'])).toBe(1)
      expect(envConfig.getNumber(['LEVEL1'])).toBe(2)
      expect(envConfig.getNumber(['LEVEL2'])).toBe(3)
    })

    it('should handle include_env with comments', () => {
      const includedEnvPath = path.resolve(configDir, 'included.env')

      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = [
        'VALUE=main',
        'include_env=included.env # This is a comment',
      ]
      mockFiles[includedEnvPath] = 'INCLUDED=value'

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.getString(['VALUE'])).toBe('main')
      expect(envConfig.getString(['INCLUDED'])).toBe('value')
    })

    it('should throw error when included file does not exist', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = `include_env=nonexistent.env`

      expect(() => new EnvConfig(configDir)).toThrow('process.exit called')

      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('not found'),
      )
    })

    it('should detect and throw error on circular include_env directives', () => {
      const file1Path = path.resolve(configDir, 'file1.env')
      const file2Path = path.resolve(configDir, 'file2.env')

      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      // Create a circular reference: .env -> file1.env -> file2.env ->
      // file1.env
      mockFiles[envPath] = ['VALUE=main', 'include_env=file1.env']
      mockFiles[file1Path] = ['VALUE1=first', 'include_env=file2.env']
      mockFiles[file2Path] = ['VALUE2=second', 'include_env=file1.env']

      expect(() => new EnvConfig(configDir)).toThrow('process.exit called')
      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('Circular include_env directive detected'),
      )
    })

    it('should detect self-referencing include_env directive', () => {
      const selfRefPath = path.resolve(configDir, 'self.env')

      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      // Create a self-referencing file
      mockFiles[envPath] = ['VALUE=main', 'include_env=self.env']
      mockFiles[selfRefPath] = ['SELF_VALUE=value', 'include_env=self.env']

      expect(() => new EnvConfig(configDir)).toThrow('process.exit called')
      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('Circular include_env directive detected'),
      )
    })

    it('should resolve relative paths correctly', () => {
      const includedEnvPath = path.resolve(configDir, 'subdir', 'included.env')

      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = `include_env=subdir/included.env`
      mockFiles[includedEnvPath] = `NESTED=value`

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.getString(['NESTED'])).toBe('value')
    })
  })

  describe('edge cases', () => {
    it('should handle empty .env file', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = ''

      const envConfig = new EnvConfig(configDir)

      const result = envConfig.getString(['key'], 'default')
      expect(result).toBe('default')
    })

    it('should fail to load missing package.json', () => {
      expect(() => new EnvConfig(configDir)).toThrow('ENOENT')
    })

    it('should handle empty key array', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: { root: 'value' },
      }
      mockFiles[envPath] = ''

      const envConfig = new EnvConfig(configDir)

      expect(() => envConfig.getString([])).toThrow('keyPath must not be empty')
    })

    it('should handle undefined and null values correctly', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = 'NULL_VALUE=null'

      const envConfig = new EnvConfig(configDir)

      const nullResult = envConfig.getString(['NULL', 'VALUE'])
      expect(nullResult).toBe('null')

      // When no env or package config exists, return the default value
      // (undefined)
      const undefResult = envConfig.getString(['NONEXISTENT', 'VALUE'])
      expect(undefResult).toBeUndefined()
    })

    it('should handle BOM (Byte Order Mark) at the beginning of .env file', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      // Include BOM at the beginning of the file
      mockFiles[envPath] = '\uFEFFTEST_VALUE=value\nSECOND_VALUE=second'

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.getString(['TEST', 'VALUE'])).toBe('value')
      expect(envConfig.getString(['SECOND', 'VALUE'])).toBe('second')
    })
  })

  describe('path', () => {
    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {
          relative_path: 'subdir/file.txt',
          absolute_path_unix: '/usr/local/bin/tool',
          absolute_path_windows: 'C:\\Program Files\\Tool\\tool.exe',
          empty_path: '',
        },
      }
      mockFiles[envPath] = [
        'ENV_RELATIVE_PATH=relative/path.txt',
        'ENV_ABSOLUTE_PATH=/home/user/project',
        'ENV_WINDOWS_PATH=D:/projects/brave',
        'ENV_EMPTY_PATH=',
      ]

      envConfig = new EnvConfig(configDir)
    })

    it('non-existent path', () => {
      expect(envConfig.getPath(['a'])).toBeUndefined()
      expect(envConfig.getPath(['a', 'b'])).toBeUndefined()
      expectRequiredNotFound(() => envConfig.requirePath(['a']))
      expectRequiredNotFound(() => envConfig.requirePath(['a', 'b']))
    })

    it('should return undefined for empty path value', () => {
      expect(envConfig.getPath(['empty_path'])).toBeUndefined()
    })

    it('should resolve relative paths to absolute paths', () => {
      const result = envConfig.getPath(['relative_path'])
      assert(typeof result === 'string')
      const expected = path.resolve(configDir, 'subdir/file.txt')
      expect(result).toBe(expected)
      expect(path.isAbsolute(result)).toBe(true)
      expect(envConfig.requirePath(['relative_path'])).toBe(expected)
    })

    it('should resolve relative paths from .env to absolute paths', () => {
      const result = envConfig.getPath(['ENV', 'RELATIVE', 'PATH'])
      assert(typeof result === 'string')
      const expected = path.resolve(configDir, 'relative/path.txt')
      expect(result).toBe(expected)
      expect(path.isAbsolute(result)).toBe(true)
      expect(envConfig.requirePath(['ENV', 'RELATIVE', 'PATH'])).toBe(expected)
    })

    if (process.platform === 'win32') {
      it('should get absolute path on windows', () => {
        expect(envConfig.getPath(['absolute_path_windows'])).toBe(
          'C:\\Program Files\\Tool\\tool.exe',
        )
        expect(envConfig.getPath(['ENV', 'WINDOWS', 'PATH'])).toBe(
          'D:\\projects\\brave',
        )
      })
    } else {
      it('should get absolute path on posix', () => {
        expect(envConfig.getPath(['absolute_path_unix'])).toBe(
          '/usr/local/bin/tool',
        )
        expect(envConfig.getPath(['ENV', 'ABSOLUTE', 'PATH'])).toBe(
          '/home/user/project',
        )
      })
    }

    it('should prioritize .env values over package.json values', () => {
      mockFiles[envPath] = 'relative_path=env/override.txt'
      envConfig = new EnvConfig(configDir)

      const result = envConfig.getPath(['relative_path'])
      const expected = path.resolve(configDir, 'env/override.txt')
      expect(result).toBe(expected)
      expect(envConfig.requirePath(['relative_path'])).toBe(expected)
    })

    it('should expand ~ to home directory', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {
          home_path: '~/.config/app',
        },
      }
      envConfig = new EnvConfig(configDir)

      const result = envConfig.getPath(['home_path'])
      const expected = path.join(os.homedir(), '.config/app')
      expect(result).toBe(expected)
      assert(typeof result === 'string')
      expect(path.isAbsolute(result)).toBe(true)
      expect(envConfig.requirePath(['home_path'])).toBe(expected)
    })

    it('should expand ~ from .env to home directory', () => {
      mockFiles[envPath] = 'HOME_PATH=~/projects/brave'
      envConfig = new EnvConfig(configDir)

      const result = envConfig.getPath(['HOME', 'PATH'])
      const expected = path.join(os.homedir(), 'projects/brave')
      expect(result).toBe(expected)
      assert(typeof result === 'string')
      expect(path.isAbsolute(result)).toBe(true)
      expect(envConfig.requirePath(['HOME', 'PATH'])).toBe(expected)
    })

    it('should handle ~ as exact home directory', () => {
      mockFiles[envPath] = 'HOME_DIR=~'
      envConfig = new EnvConfig(configDir)

      const result = envConfig.getPath(['HOME', 'DIR'])
      const expectedPosix = os.homedir()
      expect(result).toBe(expectedPosix)
      expect(envConfig.requirePath(['HOME', 'DIR'])).toBe(expectedPosix)
    })
  })
})
