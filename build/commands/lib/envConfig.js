// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict'

const assert = require('assert')
const dotenv = require('dotenv')
const fs = require('fs')
const path = require('path')
const Log = require('./logging')

/**
 * Type name for supported configuration values.
 * @typedef {'Undefined'|'Null'|'String'|'Number'|'Boolean'|'Array'|'Object'} ConfigValueType
 */

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
  /** Package.json config merged with version.
   *  @type {{config: Record<string, any>, version: string}}  */
  #packageConfig
  /** Raw variables from .env files.
   *  @type {Record<string, string>}  */
  #dotenvConfig
  /** Parsed and typed config values cache.
   *  @type {Record<string, any>}  */
  #envConfigTyped
  /** Stored default values for validation.
   *  @type {Record<string, any>}  */
  #seenDefaultValues

  /**
   * Creates a new EnvConfig instance and loads all configuration files.
   *
   * @param {string} configDir - Directory containing package.json and .env
   * files
   */
  constructor(configDir) {
    this.#packageConfig = this.#loadPackageConfig(configDir)
    this.#dotenvConfig = this.#loadDotenvConfig(configDir)

    this.#envConfigTyped = {}
    this.#seenDefaultValues = {}
  }

  /**
   * Retrieves a value from package.json configuration.
   *
   * @param {string[]} keyPath - Array of keys forming the path to the config
   * value (e.g., ['projects', 'chrome', 'version'])
   * @returns {*} The configuration value, or undefined if not found
   */
  getPackageConfig(keyPath) {
    return keyPath.reduce((obj, subkey) => obj?.[subkey], this.#packageConfig)
  }

  /**
   * Retrieves a configuration value from .env files or package.json with type
   * validation and caching.
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
    const keyJoined = keyPath.join('_')

    // Check already processed value first.
    if (keyJoined in this.#envConfigTyped) {
      assert.deepStrictEqual(
        defaultValue,
        this.#seenDefaultValues[keyJoined],
        `EnvConfig.get for key ${keyJoined} was called with a different defaultValue before`,
      )
      return this.#envConfigTyped[keyJoined]
    }

    // Store the default value for the key to check for mismatches on subsequent
    // calls to the same key.
    this.#seenDefaultValues[keyJoined] = defaultValue

    const dotenvConfigValue = this.#getDotenvConfig(
      keyJoined,
      getConfigValueType(defaultValue),
    )
    if (dotenvConfigValue !== undefined) {
      return (this.#envConfigTyped[keyJoined] = dotenvConfigValue)
    }

    const packageConfigValue = this.getPackageConfig(keyPath)
    if (packageConfigValue !== undefined) {
      return (this.#envConfigTyped[keyJoined] = packageConfigValue)
    }

    return (this.#envConfigTyped[keyJoined] = defaultValue)
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
      try {
        return JSON.parse(dotenvConfigValue)
      } catch (e) {
        return dotenvConfigValue
      }
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

    // Validate the parsed value against the expected value type.
    const dotenvConfigValueType = getConfigValueType(dotenvConfigValueParsed)
    if (dotenvConfigValueType !== expectedValueType) {
      Log.error(
        `${keyJoined} value type is invalid: expected ${expectedValueType}, got ${dotenvConfigValueType}`,
      )
      process.exit(1)
    }

    return dotenvConfigValueParsed
  }

  /**
   * Loads package.json configuration from the specified directory.
   * Merges the config section with version information.
   *
   * @private
   * @param {string} configDir - Directory containing package.json
   */
  #loadPackageConfig(configDir) {
    const configAbsolutePath = path.join(configDir, 'package.json')
    const packageJsonContent = fs.readFileSync(configAbsolutePath, 'utf8')
    const packages = JSON.parse(packageJsonContent)

    // packages.config should include version string.
    return Object.assign({}, packages.config, {
      version: packages.version,
    })
  }

  /**
   * Loads .env configuration from the specified directory.
   * Supports include_env directives for composing multiple .env files.
   * Creates a placeholder .env file if none exists.
   *
   * @private
   * @param {string} configDir - Directory containing .env file
   */
  #loadDotenvConfig(configDir) {
    const dotenvConfig = {}
    // Parse {configDir}/.env with all included env files.
    const dotenvConfigPath = path.join(configDir, '.env')
    if (fs.existsSync(dotenvConfigPath)) {
      dotenvPopulateWithIncludes(dotenvConfig, dotenvConfigPath)
    } else {
      // The .env file is used by `gn gen`. Create it if it doesn't exist.
      const defaultEnvConfigContent =
        '# This is a placeholder .env config file for the build system.\n'
        + '# See for details: https://github.com/brave/brave-browser/wiki/Build-configuration\n'
      fs.writeFileSync(dotenvConfigPath, defaultEnvConfigContent)
    }
    return dotenvConfig
  }
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
 * @param {Object} processEnv - Object to populate with parsed variables
 * @param {string} envPath - Path to the main .env file to parse
 */
function dotenvPopulateWithIncludes(processEnv, envPath) {
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

  const finalEnvContent = readEnvFile(envPath, envPath)
  dotenv.populate(processEnv, dotenv.parse(finalEnvContent))
}

/**
 * Returns a string representing the type of a value. Throws an error if the
 * value is not a supported value type.
 *
 * @param {*} value - Value to get the type of
 * @returns {ConfigValueType} Type name
 * @throws {AssertionError} If type name is not a supported value type
 */
function getConfigValueType(value) {
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

module.exports = EnvConfig
