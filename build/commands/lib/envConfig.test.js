// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs')
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

      expect(envConfig.getPackageConfig(['version'])).toBe('1.2.3')
      expect(
        envConfig.getPackageConfig(['projects', 'chrome', 'version']),
      ).toBe('120.0.0')
      expect(envConfig.getPackageConfig(['TEST', 'VALUE'])).toBe(undefined)
      expect(envConfig.getEnvConfig(['version'])).toBe('1.2.3')
      expect(envConfig.getEnvConfig(['projects', 'chrome', 'version'])).toBe(
        '120.0.0',
      )
      expect(envConfig.getEnvConfig(['TEST', 'VALUE'])).toBe('hello')
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

  describe('getPackageConfig', () => {
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
      mockFiles[envPath] = ''

      envConfig = new EnvConfig(configDir)
    })

    it('should retrieve nested config values', () => {
      const result = envConfig.getPackageConfig(['projects', 'chrome', 'tag'])
      expect(result).toBe('120.0.0')
    })

    it('should retrieve top-level config values', () => {
      const result = envConfig.getPackageConfig(['version'])
      expect(result).toBe('1.2.3')
    })

    it('should return undefined for non-existent keys', () => {
      const result = envConfig.getPackageConfig(['nonexistent', 'key'])
      expect(result).toBeUndefined()
    })

    it('should return undefined when path traverses non-object', () => {
      const result = envConfig.getPackageConfig(['version', 'nested'])
      expect(result).toBeUndefined()
    })
  })

  describe('getEnvConfig', () => {
    let envConfig

    beforeEach(() => {
      mockFiles[packageJsonPath] = {
        version: '1.2.3',
        config: {
          fallback_value: 'from_package',
        },
      }
      mockFiles[envPath] = [
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

    it('should retrieve string values from .env', () => {
      const result = envConfig.getEnvConfig(['STRING', 'VALUE'])
      expect(result).toBe('hello')
    })

    it('should parse JSON values when default is not provided', () => {
      expect(envConfig.getEnvConfig(['NUMBER', 'VALUE'])).toBe(42)
      expect(envConfig.getEnvConfig(['BOOL', 'VALUE'])).toBe(true)
      expect(envConfig.getEnvConfig(['ARRAY', 'VALUE'])).toEqual([
        'a',
        'b',
        'c',
      ])
      expect(envConfig.getEnvConfig(['OBJECT', 'VALUE'])).toEqual({
        key: 'value',
      })
      expect(envConfig.getEnvConfig(['EMPTY', 'STRING', 'VALUE'])).toBe('')
      expect(envConfig.getEnvConfig(['EMPTY', 'NUMBER', 'VALUE'])).toBe(0)
      expect(envConfig.getEnvConfig(['EMPTY', 'BOOL', 'VALUE'])).toBe(false)
      expect(envConfig.getEnvConfig(['EMPTY', 'ARRAY', 'VALUE'])).toEqual([])
      expect(envConfig.getEnvConfig(['EMPTY', 'OBJECT', 'VALUE'])).toEqual({})
    })

    it('should parse typed values', () => {
      expect(envConfig.getEnvConfig(['STRING', 'VALUE'], '')).toBe('hello')
      expect(envConfig.getEnvConfig(['NUMBER', 'VALUE'], 0)).toBe(42)
      expect(envConfig.getEnvConfig(['BOOL', 'VALUE'], false)).toBe(true)
      expect(envConfig.getEnvConfig(['ARRAY', 'VALUE'], [])).toEqual([
        'a',
        'b',
        'c',
      ])
      expect(envConfig.getEnvConfig(['OBJECT', 'VALUE'], {})).toEqual({
        key: 'value',
      })
    })

    it('should use string value as-is when default is string', () => {
      expect(envConfig.getEnvConfig(['NUMBER', 'VALUE'], 'default')).toBe('42')
      expect(envConfig.getEnvConfig(['BOOL', 'VALUE'], 'default')).toBe('true')
      expect(envConfig.getEnvConfig(['ARRAY', 'VALUE'], 'default')).toEqual(
        JSON.stringify(['a', 'b', 'c']),
      )
      expect(envConfig.getEnvConfig(['OBJECT', 'VALUE'], 'default')).toEqual(
        JSON.stringify({
          key: 'value',
        }),
      )
    })

    it('should fall back to package config when .env value not found', () => {
      const result = envConfig.getEnvConfig(['fallback_value'])
      expect(result).toBe('from_package')
    })

    it('should use default value when neither .env nor package config exist', () => {
      const result = envConfig.getEnvConfig(
        ['nonexistent', 'key'],
        'default_val',
      )
      expect(result).toBe('default_val')
    })

    it('should return the same value for the same key', () => {
      const result1 = envConfig.getEnvConfig(['OBJECT', 'VALUE'])
      const result2 = envConfig.getEnvConfig(['OBJECT', 'VALUE'])

      expect(result1).toEqual(result2)
    })

    it('should throw error when called with different default values for same key', () => {
      envConfig.getEnvConfig(['TEST', 'KEY'], 'default1')

      expect(() => {
        envConfig.getEnvConfig(['TEST', 'KEY'], 'default2')
      }).toThrow(/was called with a different defaultValue before/)
    })

    it('should allow same key with same default value', () => {
      const result1 = envConfig.getEnvConfig(['TEST', 'KEY'], 'default')
      const result2 = envConfig.getEnvConfig(['TEST', 'KEY'], 'default')

      expect(result1).toBe(result2)
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
        envConfig.getEnvConfig(['INVALID', 'JSON'], 123)
      }).toThrow('process.exit called')

      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('not JSON-parseable'),
      )
    })

    it('should throw error when type does not match default type', () => {
      expect(() => {
        envConfig.getEnvConfig(['WRONG', 'TYPE'], [])
      }).toThrow('process.exit called')

      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('value type is invalid'),
      )
    })

    it('should not throw when type matches', () => {
      const result = envConfig.getEnvConfig(['WRONG', 'TYPE'], 0)
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

      expect(envConfig.getEnvConfig(['MAIN', 'VALUE'])).toBe('main')
      expect(envConfig.getEnvConfig(['INCLUDED', 'VALUE'])).toBe('included')
      expect(envConfig.getEnvConfig(['OVERRIDE', 'VALUE'])).toBe('from_main')
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

      expect(envConfig.getEnvConfig(['MAIN'])).toBe(1)
      expect(envConfig.getEnvConfig(['LEVEL1'])).toBe(2)
      expect(envConfig.getEnvConfig(['LEVEL2'])).toBe(3)
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

      expect(envConfig.getEnvConfig(['VALUE'])).toBe('main')
      expect(envConfig.getEnvConfig(['INCLUDED'])).toBe('value')
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

      expect(envConfig.getEnvConfig(['NESTED'])).toBe('value')
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

      const result = envConfig.getEnvConfig(['key'], 'default')
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

      const result = envConfig.getPackageConfig([])
      expect(result).toEqual({ version: '1.0.0', root: 'value' })
    })

    it('should handle undefined and null values correctly', () => {
      mockFiles[packageJsonPath] = {
        version: '1.0.0',
        config: {},
      }
      mockFiles[envPath] = 'NULL_VALUE=null'

      const envConfig = new EnvConfig(configDir)

      const nullResult = envConfig.getEnvConfig(['NULL', 'VALUE'])
      expect(nullResult).toBeNull()

      // When no env or package config exists, return the default value
      // (undefined)
      const undefResult = envConfig.getEnvConfig(['NONEXISTENT', 'VALUE'])
      expect(undefResult).toBeUndefined()
    })
  })
})
