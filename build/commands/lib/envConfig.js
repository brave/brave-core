// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const assert = require('assert')
const fs = require('fs')
const path = require('path')
const { parseEnv } = require('node:util')
const Log = require('./logging')

/**
 * EnvConfig - Unified configuration loader for Brave build system
 *
 * This class manages all configuration loading for the Brave build system,
 * combining values from multiple sources with a clear priority order:
 *   1. Variables from .env files (highest priority)
 *   2. Package configuration from package.json
 *   3. Default values provided by caller (lowest priority)
 */
class EnvConfig {
  /** Package.json object.
   *  @type {Record<string, any>}  */
  #packageJson
  /** Raw variables from .env files.
   *  @type {Record<string, string>}  */
  #dotenvConfig
  /** Stored default values for assertions on the same key requests.
   *  @type {Record<string, any>}  */
  #seenDefaultValues

  /**
   * Type name for supported configuration value types.
   * @typedef {'Undefined'|'Null'|'String'|'Number'|'Boolean'|'Array'|'Object'} ConfigValueType
   */

  /**
   * Creates a new EnvConfig instance and loads all configuration files.
   *
   * @param {string} configDir - Directory containing package.json and .env
   * files
   */
  constructor(configDir) {
    this.#packageJson = EnvConfig.#loadPackageJson(configDir)
    this.#dotenvConfig = EnvConfig.#loadDotenvConfig(configDir)
    this.#seenDefaultValues = {}
  }

  /**
   * Retrieves a configuration value from .env files or package.json with type
   * validation.
   *
   * The method looks up configuration in this order:
   *   1. .env file values (key parts joined with '_', e.g.,
   *      'projects_chrome_tag')
   *   2. package.json values (using the key array as a path)
   *   3. The provided defaultValue
   *
   * Type handling:
   * - If no defaultValue is provided, attempts JSON parsing or returns string
   * - If defaultValue is a string, returns .env value as-is
   * - For other types, parses .env value as JSON and validates type matches
   *
   * @param {string[]} keyPath - Array of keys forming the config path
   * @param {*} defaultValue - Default value if config not found; also
   * determines expected type
   * @returns {*} The configuration value with appropriate type
   * @throws {AssertionError} If called with different defaultValue for same key
   * @throws {Error} If .env value cannot be parsed or has wrong type
   */
  get(keyPath, defaultValue) {
    assert.notEqual(keyPath.length, 0, 'keyPath must not be empty')
    const keyJoined = keyPath.join('_')

    this.#assertDefaultValueIsSame(keyJoined, defaultValue)
    const expectedValueType = EnvConfig.#getValueType(defaultValue)

    const dotenvConfigValue = this.#getDotenvConfig(
      keyJoined,
      expectedValueType,
    )
    if (dotenvConfigValue !== undefined) {
      return dotenvConfigValue
    }

    const packageConfigValue = this.#getPackageConfig(
      keyPath,
      expectedValueType,
    )
    if (packageConfigValue !== undefined) {
      return packageConfigValue
    }

    return defaultValue
  }

  /**
   * Returns a merged object from .env files and package.json.
   *
   * @param {string[]} keyPath - Array of keys forming the path to the config
   * value (e.g., ['projects', 'chrome', 'custom_deps'])
   * @returns {Record<string, any>} The merged object
   */
  getMergedObject(keyPath) {
    assert.notEqual(keyPath.length, 0, 'keyPath must not be empty')
    const keyJoined = keyPath.join('_')

    const dotenvConfigValue = this.#getDotenvConfig(keyJoined, 'Object') || {}
    const packageConfigValue = this.#getPackageConfig(keyPath, 'Object') || {}

    const mergedObject = { ...packageConfigValue, ...dotenvConfigValue }

    for (const [key, value] of Object.entries(this.#dotenvConfig)) {
      const keyPrefix = `${keyJoined}_`
      if (key.startsWith(keyPrefix)) {
        mergedObject[key.replace(keyPrefix, '')] =
          EnvConfig.#parseJsonOrKeepString(value)
      }
    }

    return mergedObject
  }

  /**
   * Returns the package version from package.json.
   *
   * @returns {string} The package version
   */
  getPackageVersion() {
    return this.#packageJson.version
  }

  /**
   * Retrieves a value from package.json "config" value.
   *
   * @param {string[]} keyPath - Array of keys forming the path to the config
   * value (e.g., ['projects', 'chrome', 'tag'])
   * @param {ConfigValueType} expectedValueType - Expected type of the value
   * @returns {*} The config value, or undefined if not found
   */
  #getPackageConfig(keyPath, expectedValueType) {
    const packageConfigValue = keyPath.reduce(
      (obj, subkey) => obj?.[subkey],
      this.#packageJson.config,
    )
    if (packageConfigValue === undefined) {
      return undefined
    }

    EnvConfig.#validateValueType(
      packageConfigValue,
      expectedValueType,
      () => `${keyPath.join('_')} (from package.json)`,
    )
    return packageConfigValue
  }

  /**
   * Retrieves and parses a value from .env configuration.
   *
   * @private
   * @param {string} keyJoined - The joined key path (e.g., 'projects_chrome_tag')
   * @param {ConfigValueType} expectedValueType - Expected type of the value
   * @returns {*} The parsed configuration value, or undefined if not found
   */
  #getDotenvConfig(keyJoined, expectedValueType) {
    const dotenvConfigValue = this.#dotenvConfig[keyJoined]
    if (dotenvConfigValue === undefined) {
      return undefined
    }

    // Parse as JSON or return a string if no default value is provided.
    if (expectedValueType === 'Undefined') {
      return EnvConfig.#parseJsonOrKeepString(dotenvConfigValue)
    }

    // Use the value as is if the expected value type is a string.
    if (expectedValueType === 'String') {
      return dotenvConfigValue
    }

    // Parse as JSON if the expected value type is not a string.
    let dotenvConfigValueParsed
    try {
      dotenvConfigValueParsed = JSON.parse(dotenvConfigValue)
    } catch (e) {
      Log.error(
        `${keyJoined} value is not JSON-parseable: ${dotenvConfigValue}\n${e.message}`,
      )
      process.exit(1)
    }

    EnvConfig.#validateValueType(
      dotenvConfigValueParsed,
      expectedValueType,
      () => `${keyJoined} (from .env)`,
    )
    return dotenvConfigValueParsed
  }

  /**
   * Loads package.json file from the specified directory.
   *
   * @private
   * @param {string} configDir - Directory containing package.json
   * @returns {Record<string, any>} The parsed package.json
   */
  static #loadPackageJson(configDir) {
    const packageJsonPath = path.join(configDir, 'package.json')
    const packageJsonContent = fs.readFileSync(packageJsonPath, 'utf8')
    const packageJson = JSON.parse(packageJsonContent)
    EnvConfig.#validateValueType(
      packageJson.version,
      'String',
      () => 'package.json version',
    )
    EnvConfig.#validateValueType(
      packageJson.config,
      'Object',
      () => 'package.json config',
    )
    return packageJson
  }

  /**
   * Loads .env configuration from the specified directory.
   * Supports include_env directives for composing multiple .env files.
   * Creates a placeholder .env file if none exists.
   *
   * @private
   * @param {string} configDir - Directory containing .env file
   */
  static #loadDotenvConfig(configDir) {
    let dotenvConfig = {}
    // Parse {configDir}/.env with all included env files.
    const dotenvConfigPath = path.join(configDir, '.env')
    if (fs.existsSync(dotenvConfigPath)) {
      dotenvConfig = EnvConfig.#parseEnvFileWithIncludes(dotenvConfigPath)
    } else {
      // The .env file is used by `gn gen`. Create it if it doesn't exist.
      const defaultEnvConfigContent =
        '# This is a placeholder .env config file for the build system.\n'
        + '# See for details: https://github.com/brave/brave-browser/wiki/Build-configuration\n'
      fs.writeFileSync(dotenvConfigPath, defaultEnvConfigContent)
    }
    return dotenvConfig
  }

  /**
   * Reads a .env file and recursively includes other .env files using the
   * include_env=<path> directive.
   *
   * The include_env directive allows composing multiple .env files together.
   * Included files are processed recursively, allowing nested includes.
   *
   * Format: include_env=path/to/file.env
   *
   * @param {string} envPath - Path to the main .env file to parse
   * @returns {Record<string, string>} The parsed .env file
   */
  static #parseEnvFileWithIncludes(envPath) {
    const seenFiles = new Set()
    function readEnvFile(filePath, fromFile) {
      if (seenFiles.has(filePath)) {
        Log.error(
          `Circular include_env directive detected: ${filePath} from ${fromFile}`,
        )
        process.exit(1)
      }
      seenFiles.add(filePath)

      if (!fs.existsSync(filePath)) {
        Log.error(`Error loading .env (not found) from: ${filePath}`)
        process.exit(1)
      }

      const envContent = fs.readFileSync(filePath, 'utf8')
      const lines = envContent.split('\n')
      let result = ''

      lines.forEach((line) => {
        const includeEnvMatch = line.match(/^include_env=([^#]+)(?:#.*)?$/)
        if (includeEnvMatch) {
          const includePath = includeEnvMatch[1].trim()
          const resolvedPath = path.resolve(path.dirname(filePath), includePath)
          result += readEnvFile(resolvedPath, filePath)
        } else {
          result += line + '\n'
        }
      })

      return result
    }

    return parseEnv(readEnvFile(envPath, envPath))
  }

  /**
   * Asserts that the defaultValue is the same as the previous one.
   *
   * @private
   * @param {string} key - The key (e.g., 'projects_chrome_tag')
   * @param {*} defaultValue - The default value to assert
   */
  #assertDefaultValueIsSame(key, defaultValue) {
    if (key in this.#seenDefaultValues) {
      assert.deepStrictEqual(
        defaultValue,
        this.#seenDefaultValues[key],
        `EnvConfig for key ${key} was requested with a different defaultValue`,
      )
    } else {
      this.#seenDefaultValues[key] = defaultValue
    }
  }

  /**
   * Validates the value against the expected value type.
   *
   * @private
   * @param {*} value - The value to validate
   * @param {ConfigValueType} expectedValueType - Expected type of the value
   * @param {function(): string} valueDescCallback - Callback to get the
   * description of the value
   */
  static #validateValueType(value, expectedValueType, valueDescCallback) {
    if (expectedValueType === 'Undefined') {
      return
    }

    const valueType = EnvConfig.#getValueType(value)
    if (valueType !== expectedValueType) {
      Log.error(
        `${valueDescCallback()} value type is invalid: expected ${expectedValueType}, got ${valueType}`,
      )
      process.exit(1)
    }
  }

  /**
   * Returns a string representing the type of a value. Throws an error if the
   * value is not a supported value type.
   *
   * @param {*} value - Value to get the type of
   * @returns {ConfigValueType} Type name
   * @throws {AssertionError} If type name is not a supported value type
   */
  static #getValueType(value) {
    if (value === undefined) {
      return 'Undefined'
    }
    if (value === null) {
      return 'Null'
    }

    const typeName = value.constructor.name

    // Only allow JSON-parseable types
    const validTypes = ['String', 'Number', 'Boolean', 'Array', 'Object']
    assert(
      validTypes.includes(typeName),
      `Unsupported type: ${typeName}. Only ${validTypes.join(', ')} are allowed.`,
    )

    return typeName
  }

  /**
   * Parses a value as JSON or returns it as a string if it is not
   * JSON-parseable.
   *
   * @param {*} value - The value to parse
   * @returns {*} The parsed value or the original value if it is not
   * JSON-parseable
   */
  static #parseJsonOrKeepString(value) {
    try {
      return JSON.parse(value)
    } catch (e) {
      return value
    }
  }
}

module.exports = EnvConfig
