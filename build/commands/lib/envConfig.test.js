// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs')
const os = require('os')
const path = require('path')
const EnvConfig = require('./envConfig')

// Mock the logging module
jest.mock('./logging', () => ({
  error: jest.fn(),
}))

const Log = require('./logging')

describe('EnvConfig', () => {
  const configDir = '/path/to/config'
  const packageJsonPath = path.join(configDir, 'package.json')
  const envPath = path.join(configDir, '.env')

  let mockFiles = {}
  let originalExistsSync
  let originalReadFileSync
  let originalWriteFileSync

  beforeEach(() => {
    jest.clearAllMocks()
    mockFiles = {}

    // Mock fs.existsSync
    originalExistsSync = fs.existsSync
    fs.existsSync = jest.fn((filePath) => {
      return mockFiles.hasOwnProperty(filePath)
    })

    // Mock fs.readFileSync
    originalReadFileSync = fs.readFileSync
    fs.readFileSync = jest.fn((filePath, encoding) => {
      if (!mockFiles.hasOwnProperty(filePath)) {
        throw new Error(`ENOENT: no such file or directory, open '${filePath}'`)
      }
      const f = mockFiles[filePath]
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
    originalWriteFileSync = fs.writeFileSync
    fs.writeFileSync = jest.fn((filePath, data) => {
      mockFiles[filePath] = data
    })

    // Mock process.exit to prevent tests from exiting
    jest.spyOn(process, 'exit').mockImplementation(() => {
      throw new Error('process.exit called')
    })
  })

  afterEach(() => {
    fs.existsSync = originalExistsSync
    fs.readFileSync = originalReadFileSync
    fs.writeFileSync = originalWriteFileSync
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
      expect(envConfig.get(['projects', 'chrome', 'version'])).toBe('120.0.0')
      expect(envConfig.get(['TEST', 'VALUE'])).toBe('hello')
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

  describe('get without .env', () => {
    let envConfig

    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.2.3',
        config: {
          projects: {
            chrome: {
              tag: '120.0.0',
            },
          },
        },
      }

      envConfig = new EnvConfig(configDir)
    })

    it('should retrieve version', () => {
      const result = envConfig.getPackageVersion()
      expect(result).toBe('1.2.3')
    })

    it('should retrieve config values', () => {
      const result = envConfig.get(['projects', 'chrome', 'tag'])
      expect(result).toBe('120.0.0')
    })

    it('should return undefined for non-existent keys', () => {
      const result = envConfig.get(['nonexistent', 'key'])
      expect(result).toBeUndefined()
    })

    it('should return undefined when path traverses non-object', () => {
      const result = envConfig.get(['config', 'nested'])
      expect(result).toBeUndefined()
    })
  })

  describe('get', () => {
    let envConfig

    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.2.3',
        config: {
          fallback_value: 'from_package',
        },
      }
      mockFiles[envPath] = [
        'version=4.5.6',
        'STRING_VALUE=hello',
        'NUMBER_VALUE=42',
        'BOOL_VALUE=true',
        'ARRAY_VALUE=["a","b","c"]',
        'OBJECT_VALUE={"key":"value"}',
        'EMPTY_STRING_VALUE=""',
        'EMPTY_NUMBER_VALUE=0',
        'EMPTY_BOOL_VALUE=false',
        'EMPTY_ARRAY_VALUE=[]',
        'EMPTY_OBJECT_VALUE={}',
      ]

      envConfig = new EnvConfig(configDir)
    })

    it('version should not override package.json version', () => {
      const result = envConfig.getPackageVersion()
      expect(result).toBe('1.2.3')
    })

    it('should retrieve string values from .env', () => {
      const result = envConfig.get(['STRING', 'VALUE'])
      expect(result).toBe('hello')
    })

    it('should parse JSON values when default is not provided', () => {
      expect(envConfig.get(['NUMBER', 'VALUE'])).toBe(42)
      expect(envConfig.get(['BOOL', 'VALUE'])).toBe(true)
      expect(envConfig.get(['ARRAY', 'VALUE'])).toEqual(['a', 'b', 'c'])
      expect(envConfig.get(['OBJECT', 'VALUE'])).toEqual({
        key: 'value',
      })
      expect(envConfig.get(['EMPTY', 'STRING', 'VALUE'])).toBe('')
      expect(envConfig.get(['EMPTY', 'NUMBER', 'VALUE'])).toBe(0)
      expect(envConfig.get(['EMPTY', 'BOOL', 'VALUE'])).toBe(false)
      expect(envConfig.get(['EMPTY', 'ARRAY', 'VALUE'])).toEqual([])
      expect(envConfig.get(['EMPTY', 'OBJECT', 'VALUE'])).toEqual({})
    })

    it('should parse typed values', () => {
      expect(envConfig.get(['STRING', 'VALUE'], '')).toBe('hello')
      expect(envConfig.get(['NUMBER', 'VALUE'], 0)).toBe(42)
      expect(envConfig.get(['BOOL', 'VALUE'], false)).toBe(true)
      expect(envConfig.get(['ARRAY', 'VALUE'], [])).toEqual(['a', 'b', 'c'])
      expect(envConfig.get(['OBJECT', 'VALUE'], {})).toEqual({
        key: 'value',
      })
    })

    it('should use string value as-is when default is string', () => {
      expect(envConfig.get(['NUMBER', 'VALUE'], 'default')).toBe('42')
      expect(envConfig.get(['BOOL', 'VALUE'], 'default')).toBe('true')
      expect(envConfig.get(['ARRAY', 'VALUE'], 'default')).toEqual(
        JSON.stringify(['a', 'b', 'c']),
      )
      expect(envConfig.get(['OBJECT', 'VALUE'], 'default')).toEqual(
        JSON.stringify({
          key: 'value',
        }),
      )
    })

    it('should fall back to package config when .env value not found', () => {
      const result = envConfig.get(['fallback_value'])
      expect(result).toBe('from_package')
    })

    it('should use default value when neither .env nor package config exist', () => {
      const result = envConfig.get(['nonexistent', 'key'], 'default_val')
      expect(result).toBe('default_val')
    })

    it('should return the same value for the same key', () => {
      const result1 = envConfig.get(['OBJECT', 'VALUE'])
      const result2 = envConfig.get(['OBJECT', 'VALUE'])

      expect(result1).toEqual(result2)
    })

    it('should throw error when called with different default values for same key', () => {
      envConfig.get(['TEST', 'KEY'], 'default1')

      expect(() => {
        envConfig.get(['TEST', 'KEY'], 'default2')
      }).toThrow(/was requested with a different defaultValue/)
    })

    it('should allow same key with same default value', () => {
      const result1 = envConfig.get(['TEST', 'KEY'], 'default')
      const result2 = envConfig.get(['TEST', 'KEY'], 'default')

      expect(result1).toBe(result2)
    })
  })

  describe('getMergedObject', () => {
    let envConfig

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

  describe('type validation', () => {
    let envConfig

    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = ['INVALID_JSON=not json', 'WRONG_TYPE=42']

      envConfig = new EnvConfig(configDir)
    })

    it('should throw error when JSON parsing fails with non-string default', () => {
      expect(() => {
        envConfig.get(['INVALID', 'JSON'], 123)
      }).toThrow('process.exit called')

      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('not JSON-parseable'),
      )
    })

    it('should throw error when type does not match default type', () => {
      expect(() => {
        envConfig.get(['WRONG', 'TYPE'], [])
      }).toThrow('process.exit called')

      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('value type is invalid'),
      )
    })

    it('should not throw when type matches', () => {
      const result = envConfig.get(['WRONG', 'TYPE'], 0)
      expect(result).toBe(42)
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

      expect(envConfig.get(['MAIN', 'VALUE'])).toBe('main')
      expect(envConfig.get(['INCLUDED', 'VALUE'])).toBe('included')
      expect(envConfig.get(['OVERRIDE', 'VALUE'])).toBe('from_main')
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

      expect(envConfig.get(['MAIN'])).toBe(1)
      expect(envConfig.get(['LEVEL1'])).toBe(2)
      expect(envConfig.get(['LEVEL2'])).toBe(3)
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

      expect(envConfig.get(['VALUE'])).toBe('main')
      expect(envConfig.get(['INCLUDED'])).toBe('value')
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

      expect(envConfig.get(['NESTED'])).toBe('value')
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

      const result = envConfig.get(['key'], 'default')
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

      expect(() => envConfig.get([])).toThrow('keyPath must not be empty')
    })

    it('should handle undefined and null values correctly', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = 'NULL_VALUE=null'

      const envConfig = new EnvConfig(configDir)

      const nullResult = envConfig.get(['NULL', 'VALUE'])
      expect(nullResult).toBeNull()

      // When no env or package config exists, return the default value
      // (undefined)
      const undefResult = envConfig.get(['NONEXISTENT', 'VALUE'])
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

      expect(envConfig.get(['TEST', 'VALUE'])).toBe('value')
      expect(envConfig.get(['SECOND', 'VALUE'])).toBe('second')
    })
  })

  describe('getPath', () => {
    let envConfig

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

    it('should return undefined for non-existent path', () => {
      const result = envConfig.getPath(['nonexistent', 'path'])
      expect(result).toBeUndefined()
    })

    it('should return undefined for empty path value', () => {
      const result = envConfig.getPath(['empty_path'])
      expect(result).toBeUndefined()
    })

    it('should resolve relative paths to absolute paths', () => {
      const result = envConfig.getPath(['relative_path'])
      const expected = path.resolve(configDir, 'subdir/file.txt')
      expect(result).toBe(expected)
      expect(path.isAbsolute(result)).toBe(true)
    })

    it('should resolve relative paths from .env to absolute paths', () => {
      const result = envConfig.getPath(['ENV', 'RELATIVE', 'PATH'])
      const expected = path.resolve(configDir, 'relative/path.txt')
      expect(result).toBe(expected)
      expect(path.isAbsolute(result)).toBe(true)
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
      expect(path.isAbsolute(result)).toBe(true)
    })

    it('should expand ~ from .env to home directory', () => {
      mockFiles[envPath] = 'HOME_PATH=~/projects/brave'
      envConfig = new EnvConfig(configDir)

      const result = envConfig.getPath(['HOME', 'PATH'])
      const expected = path.join(os.homedir(), 'projects/brave')
      expect(result).toBe(expected)
      expect(path.isAbsolute(result)).toBe(true)
    })

    it('should handle ~ as exact home directory', () => {
      mockFiles[envPath] = 'HOME_DIR=~'
      envConfig = new EnvConfig(configDir)

      const result = envConfig.getPath(['HOME', 'DIR'])
      const expectedPosix = os.homedir()
      expect(result).toBe(expectedPosix)
    })
  })
})
