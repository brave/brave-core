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
      return mockFiles[filePath]
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
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.2.3',
        config: {
          projects: {
            chrome: {
              version: '120.0.0',
            },
          },
        },
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)
      mockFiles[envPath] = 'TEST_VALUE=hello'

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.packageConfig).toBeDefined()
      expect(envConfig.packageConfig.version).toBe('1.2.3')
      expect(envConfig.envConfig).toBeDefined()
    })

    it('should create .env file if it does not exist', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

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
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.2.3',
        config: {
          projects: {
            chrome: {
              version: '120.0.0',
              tag: 'v120',
            },
          },
          target_arch: 'x64',
        },
      }

      mockFiles[envPath] = ''
      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      envConfig = new EnvConfig(configDir)
    })

    it('should retrieve nested config values', () => {
      const result = envConfig.getPackageConfig([
        'projects',
        'chrome',
        'version',
      ])
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
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.2.3',
        config: {
          fallback_value: 'from_package',
        },
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)
      mockFiles[envPath] = [
        'STRING_VALUE=hello',
        'NUMBER_VALUE=42',
        'BOOL_VALUE=true',
        'ARRAY_VALUE=["a","b","c"]',
        'OBJECT_VALUE={"key":"value"}',
      ].join('\n')

      envConfig = new EnvConfig(configDir)
    })

    it('should retrieve string values from .env', () => {
      const result = envConfig.getEnvConfig(['STRING', 'VALUE'])
      expect(result).toBe('hello')
    })

    it('should parse JSON values when default is not provided', () => {
      const result = envConfig.getEnvConfig(['NUMBER', 'VALUE'])
      expect(result).toBe(42)
    })

    it('should parse boolean values', () => {
      const result = envConfig.getEnvConfig(['BOOL', 'VALUE'], false)
      expect(result).toBe(true)
    })

    it('should parse array values', () => {
      const result = envConfig.getEnvConfig(['ARRAY', 'VALUE'], [])
      expect(result).toEqual(['a', 'b', 'c'])
    })

    it('should parse object values', () => {
      const result = envConfig.getEnvConfig(['OBJECT', 'VALUE'], {})
      expect(result).toEqual({ key: 'value' })
    })

    it('should use string value as-is when default is string', () => {
      const result = envConfig.getEnvConfig(['NUMBER', 'VALUE'], 'default')
      expect(result).toBe('42')
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
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)
      mockFiles[envPath] = ['INVALID_JSON=not json', 'WRONG_TYPE=42'].join('\n')

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
      envConfig.envConfig.CORRECT_TYPE = '42'

      const result = envConfig.getEnvConfig(['CORRECT', 'TYPE'], 0)
      expect(result).toBe(42)
    })
  })

  describe('include_env directive', () => {
    it('should handle include_env directive in .env files', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')
      const includedEnvPath = path.resolve(configDir, 'included.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      mockFiles[envPath] = [
        'MAIN_VALUE=main',
        'include_env=included.env',
        'OVERRIDE_VALUE=from_main',
      ].join('\n')

      mockFiles[includedEnvPath] = [
        'INCLUDED_VALUE=included',
        'OVERRIDE_VALUE=from_included',
      ].join('\n')

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.envConfig.MAIN_VALUE).toBe('main')
      expect(envConfig.envConfig.INCLUDED_VALUE).toBe('included')
      expect(envConfig.envConfig.OVERRIDE_VALUE).toBe('from_main')
    })

    it('should handle nested include_env directives', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')
      const included1Path = path.resolve(configDir, 'included1.env')
      const included2Path = path.resolve(configDir, 'included2.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      mockFiles[envPath] = ['MAIN=1', 'include_env=included1.env'].join('\n')

      mockFiles[included1Path] = ['LEVEL1=2', 'include_env=included2.env'].join(
        '\n',
      )

      mockFiles[included2Path] = `LEVEL2=3`

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.envConfig.MAIN).toBe('1')
      expect(envConfig.envConfig.LEVEL1).toBe('2')
      expect(envConfig.envConfig.LEVEL2).toBe('3')
    })

    it('should handle include_env with comments', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')
      const includedEnvPath = path.resolve(configDir, 'included.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      mockFiles[envPath] = [
        'VALUE=main',
        'include_env=included.env # This is a comment',
      ].join('\n')

      mockFiles[includedEnvPath] = 'INCLUDED=value'

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.envConfig.VALUE).toBe('main')
      expect(envConfig.envConfig.INCLUDED).toBe('value')
    })

    it('should throw error when included file does not exist', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      mockFiles[envPath] = `include_env=nonexistent.env`

      expect(() => new EnvConfig(configDir)).toThrow('process.exit called')

      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('not found'),
      )
    })

    it('should detect and throw error on circular include_env directives', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')
      const file1Path = path.resolve(configDir, 'file1.env')
      const file2Path = path.resolve(configDir, 'file2.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      // Create a circular reference: .env -> file1.env -> file2.env -> file1.env
      mockFiles[envPath] = ['VALUE=main', 'include_env=file1.env'].join('\n')
      mockFiles[file1Path] = ['VALUE1=first', 'include_env=file2.env'].join(
        '\n',
      )
      mockFiles[file2Path] = ['VALUE2=second', 'include_env=file1.env'].join(
        '\n',
      )

      expect(() => new EnvConfig(configDir)).toThrow('process.exit called')
      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('Circular include_env directive detected'),
      )
    })

    it('should detect self-referencing include_env directive', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')
      const selfRefPath = path.resolve(configDir, 'self.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      // Create a self-referencing file
      mockFiles[envPath] = ['VALUE=main', 'include_env=self.env'].join('\n')
      mockFiles[selfRefPath] = [
        'SELF_VALUE=value',
        'include_env=self.env',
      ].join('\n')

      expect(() => new EnvConfig(configDir)).toThrow('process.exit called')
      expect(Log.error).toHaveBeenCalledWith(
        expect.stringContaining('Circular include_env directive detected'),
      )
    })

    it('should resolve relative paths correctly', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')
      const includedEnvPath = path.resolve(configDir, 'subdir', 'included.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)

      mockFiles[envPath] = `include_env=subdir/included.env`
      mockFiles[includedEnvPath] = `NESTED=value`

      const envConfig = new EnvConfig(configDir)

      expect(envConfig.envConfig.NESTED).toBe('value')
    })
  })

  describe('edge cases', () => {
    it('should handle empty .env file', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)
      mockFiles[envPath] = ''

      const envConfig = new EnvConfig(configDir)

      const result = envConfig.getEnvConfig(['key'], 'default')
      expect(result).toBe('default')
    })

    it('should fail to load missing package.json', () => {
      const configDir = '/path/to/config'

      expect(() => new EnvConfig(configDir)).toThrow('ENOENT')
    })

    it('should handle empty key array', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.0.0',
        config: { root: 'value' },
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)
      mockFiles[envPath] = ''

      const envConfig = new EnvConfig(configDir)

      const result = envConfig.getPackageConfig([])
      expect(result).toEqual({ version: '1.0.0', root: 'value' })
    })

    it('should handle undefined and null values correctly', () => {
      const configDir = '/path/to/config'
      const packageJsonPath = path.join(configDir, 'package.json')
      const envPath = path.join(configDir, '.env')

      const packageData = {
        version: '1.0.0',
        config: {},
      }

      mockFiles[packageJsonPath] = JSON.stringify(packageData)
      mockFiles[envPath] = 'NULL_VALUE=null'

      const envConfig = new EnvConfig(configDir)

      const nullResult = envConfig.getEnvConfig(['NULL', 'VALUE'])
      expect(nullResult).toBeNull()

      // When no env or package config exists, return the default value (undefined)
      const undefResult = envConfig.getEnvConfig(['NONEXISTENT', 'VALUE'])
      expect(undefResult).toBeUndefined()
    })
  })
})
