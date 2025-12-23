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
 * EnvConfig - Unified configuration loader for Brave build system
 *
 * This class manages all configuration loading for the Brave build system,
 * combining values from multiple sources with a clear priority order:
 *   1. Environment variables from .env files (highest priority)
 *   2. Package configuration from package.json
 *   3. Default values provided by caller (lowest priority)
 *
 * Features:
 * - Loads package.json configuration on construction
 * - Parses .env files with support for include_env directives
 * - Type validation and automatic JSON parsing of config values
 * - Caching for performance optimization
 * - Validation to prevent inconsistent default values across calls
 *
 * Usage: const envConfig = new EnvConfig('/path/to/brave/core') const value =
 *   envConfig.getEnvConfig(['projects', 'chrome', 'tag']) const pkgValue =
 *   envConfig.getPackageConfig(['version'])
 */
class EnvConfig {
  /**
   * Creates a new EnvConfig instance and loads all configuration files.
   *
   * @param {string} configDir - Directory containing package.json and .env
   * files
   */
  constructor(configDir) {
    this.packageConfig = this.#loadPackageConfig(configDir)
    this.envConfig = this.#loadEnvConfig(configDir)

    this.envConfigTyped = {}
    this.seenDefaultValues = {}
  }

  /**
   * Retrieves a value from package.json configuration.
   *
   * @param {string[]} key - Array of keys forming the path to the config value
   *                         (e.g., ['projects', 'chrome', 'version'])
   * @returns {*} The configuration value, or undefined if not found
   */
  getPackageConfig(key) {
    return key.reduce((obj, key) => obj?.[key], this.packageConfig)
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
   * Results are cached for performance. If called multiple times with the same
   * key but different defaultValues, an assertion error is thrown.
   *
   * @param {string[]} key - Array of keys forming the config path
   * @param {*} [defaultValue=undefined] - Default value if config not found;
   *                                       also determines expected type
   * @returns {*} The configuration value with appropriate type
   * @throws {AssertionError} If called with different defaultValue for same key
   * @throws {Error} If .env value cannot be parsed or has wrong type
   */
  getEnvConfig(key, defaultValue) {
    const keyJoined = key.join('_')

    // Check already processed value first.
    if (keyJoined in this.envConfigTyped) {
      assert.deepStrictEqual(
        defaultValue,
        this.seenDefaultValues[keyJoined],
        `getEnvConfig for key ${keyJoined} was called with a different defaultValue before`,
      )
      return this.envConfigTyped[keyJoined]
    }

    // Store the default value for the key to check for mismatches on subsequent
    // calls to the same key.
    this.seenDefaultValues[keyJoined] = defaultValue

    // Look for .env value.
    const envConfigValue = this.envConfig[keyJoined]
    if (envConfigValue !== undefined) {
      // Parse as JSON or return a string if no default value is provided.
      if (defaultValue === undefined) {
        try {
          return (this.envConfigTyped[keyJoined] = JSON.parse(envConfigValue))
        } catch (e) {
          return (this.envConfigTyped[keyJoined] = envConfigValue)
        }
      }

      // Use the value as is if the default value is a string.
      const defaultValueType = getValueType(defaultValue)
      if (defaultValueType === 'String') {
        return (this.envConfigTyped[keyJoined] = envConfigValue)
      }

      // Parse as JSON if the default value is not a string.
      let envConfigValueParsed
      try {
        envConfigValueParsed = JSON.parse(envConfigValue)
      } catch (e) {
        Log.error(
          `${keyJoined} value is not JSON-parseable: ${envConfigValue}\n${e.message}`,
        )
        process.exit(1)
      }

      // Validate the parsed value against the default value type.
      const envConfigValueType = getValueType(envConfigValueParsed)
      if (envConfigValueType !== defaultValueType) {
        Log.error(
          `${keyJoined} value type is invalid: expected ${defaultValueType}, got ${envConfigValueType}`,
        )
        process.exit(1)
      }

      return (this.envConfigTyped[keyJoined] = envConfigValueParsed)
    }

    const packageConfigValue = this.getPackageConfig(key)
    if (packageConfigValue !== undefined) {
      return (this.envConfigTyped[keyJoined] = packageConfigValue)
    }

    return (this.envConfigTyped[keyJoined] = defaultValue)
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
  #loadEnvConfig(configDir) {
    const envConfig = {}
    // Parse {configDir}/.env with all included env files.
    const envConfigPath = path.join(configDir, '.env')
    if (fs.existsSync(envConfigPath)) {
      dotenvPopulateWithIncludes(envConfig, envConfigPath)
    } else {
      // The .env file is used by `gn gen`. Create it if it doesn't exist.
      const defaultEnvConfigContent =
        '# This is a placeholder .env config file for the build system.\n'
        + '# See for details: https://github.com/brave/brave-browser/wiki/Build-configuration\n'
      fs.writeFileSync(envConfigPath, defaultEnvConfigContent)
    }
    return envConfig
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
 * @param {Object} processEnv - Object to populate with parsed environment
 * variables
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
 * Returns a string representing the type of a value.
 * Used for type validation in getEnvConfig.
 *
 * @param {*} value - Value to get the type of
 * @returns {string} Type name ('undefined', 'null', or constructor name)
 */
function getValueType(value) {
  if (value === undefined) {
    return 'undefined'
  }
  if (value === null) {
    return 'null'
  }
  return value.constructor.name
}

module.exports = EnvConfig
