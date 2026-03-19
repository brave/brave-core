// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import assert from 'node:assert'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { parseEnv } from 'node:util'
import * as Log from './log.ts'

/**
 * EnvConfig - Unified configuration loader for Brave build system
 *
 * This class manages all configuration loading for the Brave build system,
 * combining values from multiple sources with a clear priority order:
 *   1. Variables from .env files (highest priority)
 *   2. Package configuration from package.json
 *   3. Default values provided by caller (lowest priority)
 */
export default class EnvConfig {
  // Directory containing package.json and .env files.
  #configDir: string
  // Parsed package.json file contents.
  #packageJson: Record<string, any>
  // Raw variables from .env files.
  #dotenvConfig: NodeJS.Dict<string>

  /**
   * Creates a new EnvConfig instance and loads all configuration files.
   *
   * @param configDir - Directory containing package.json and .env files
   */
  constructor(configDir: string) {
    this.#configDir = configDir
    this.#packageJson = EnvConfig.#loadPackageJson(configDir)
    this.#dotenvConfig = EnvConfig.#loadDotenvConfig(configDir)
  }

  /**
   * Returns a boolean config value if present. If `defaultValue` is provided,
   * returns it when the config value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @param defaultValue - Optional fallback value
   * @returns The boolean config value, the fallback, or `undefined`
   */
  getBoolean(keyPath: string[]): boolean | undefined
  getBoolean(keyPath: string[], defaultValue: boolean): boolean
  getBoolean(keyPath: string[], defaultValue?: boolean): boolean | undefined {
    return this.#getOfType(keyPath, 'Boolean') ?? defaultValue
  }

  /**
   * Returns a number config value if present. If `defaultValue` is provided,
   * returns it when the config value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @param defaultValue - Optional fallback value
   * @returns The number config value, the fallback, or `undefined`
   */
  getNumber(keyPath: string[]): number | undefined
  getNumber(keyPath: string[], defaultValue: number): number
  getNumber(keyPath: string[], defaultValue?: number): number | undefined {
    return this.#getOfType(keyPath, 'Number') ?? defaultValue
  }

  /**
   * Returns a string config value if present. If `defaultValue` is provided,
   * returns it when the config value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @param defaultValue - Optional fallback value
   * @returns The string config value, the fallback, or `undefined`
   */
  getString(keyPath: string[]): string | undefined
  getString(keyPath: string[], defaultValue: string): string
  getString(keyPath: string[], defaultValue?: string): string | undefined {
    return this.#getOfType(keyPath, 'String') ?? defaultValue
  }

  /**
   * Returns an array config value if present. If `defaultValue` is provided,
   * returns it when the config value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @param defaultValue - Optional fallback value
   * @returns The array config value, the fallback, or `undefined`
   */
  getArray(keyPath: string[]): any[] | undefined
  getArray(keyPath: string[], defaultValue: any[]): any[]
  getArray(keyPath: string[], defaultValue?: any[]): any[] | undefined {
    return this.#getOfType(keyPath, 'Array') ?? defaultValue
  }

  /**
   * Returns an object config value if present. If `defaultValue` is provided,
   * returns it when the config value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @param defaultValue - Optional fallback value
   * @returns The object config value, the fallback, or `undefined`
   */
  getObject(keyPath: string[]): Record<string, any> | undefined
  getObject(
    keyPath: string[],
    defaultValue: Record<string, any>,
  ): Record<string, any>
  getObject(
    keyPath: string[],
    defaultValue?: Record<string, any>,
  ): Record<string, any> | undefined {
    return this.#getOfType(keyPath, 'Object') ?? defaultValue
  }

  /**
   * Returns a config value of any type (parsed as JSON if possible, otherwise
   * returned as a string) if present.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The config value, or `undefined` if not found
   */
  getAny(keyPath: string[]): any {
    return this.#getOfType(keyPath, 'Any')
  }

  /**
   * Returns a required boolean config value.
   *
   * Logs an error and exits if the value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The boolean config value
   */
  requireBoolean(keyPath: string[]): boolean {
    return this.#requireOfType(keyPath, 'Boolean')
  }

  /**
   * Returns a required number config value.
   *
   * Logs an error and exits if the value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The number config value
   */
  requireNumber(keyPath: string[]): number {
    return this.#requireOfType(keyPath, 'Number')
  }

  /**
   * Returns a required string config value.
   *
   * Logs an error and exits if the value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The string config value
   */
  requireString(keyPath: string[]): string {
    return this.#requireOfType(keyPath, 'String')
  }

  /**
   * Returns a required array config value.
   *
   * Logs an error and exits if the value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The array config value
   */
  requireArray(keyPath: string[]): any[] {
    return this.#requireOfType(keyPath, 'Array')
  }

  /**
   * Returns a required object config value.
   *
   * Logs an error and exits if the value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The object config value
   */
  requireObject(keyPath: string[]): Record<string, any> {
    return this.#requireOfType(keyPath, 'Object')
  }

  /**
   * Returns a merged object from .env files and package.json.
   *
   * @param keyPath - Array of keys forming the path to the config value (e.g.,
   * ['projects', 'chrome', 'custom_deps'])
   * @returns The merged object
   */
  getMergedObject(keyPath: string[]): Record<string, any> {
    const keyJoined = EnvConfig.#joinKeyPath(keyPath)

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
   * Returns an absolute path from a configuration value. Relative paths are
   * resolved relative to the *initial config directory*. Paths starting with
   * `~` are expanded to the user's home directory.
   *
   * Values from `include_env` configs are resolved relative to the same
   * *initial config directory*, not the included file's location.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The resolved absolute path, or undefined
   */
  getPath(keyPath: string[]): string | undefined {
    let pathValue = this.getString(keyPath)
    if (!pathValue) {
      return undefined
    }

    // Expand ~ to home directory.
    if (pathValue.startsWith('~')) {
      pathValue = path.join(os.homedir(), pathValue.slice(1))
    }

    // If the path is relative, resolve it relative to the config directory.
    if (!path.isAbsolute(pathValue)) {
      pathValue = path.resolve(this.#configDir, pathValue)
    }

    // Normalize the path.
    return path.normalize(pathValue)
  }

  /**
   * Returns an absolute path from a configuration value. Relative paths are
   * resolved relative to the *initial config directory*. Paths starting with
   * `~` are expanded to the user's home directory.
   *
   * Values from `include_env` configs are resolved relative to the same
   * *initial config directory*, not the included file's location.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The resolved absolute path
   */
  requirePath(keyPath: string[]): string {
    const pathValue = this.getPath(keyPath)
    if (pathValue === undefined) {
      Log.error(
        `Required config value ${EnvConfig.#joinKeyPath(keyPath)} is not set`,
      )
      process.exit(1)
    }
    return pathValue
  }

  /**
   * Returns the package version from package.json.
   *
   * @returns The package version
   */
  getPackageVersion(): string {
    return this.#packageJson.version
  }

  /**
   * Returns a typed config value if present in any source.
   *
   * @param keyPath - Array of keys forming the config path
   * @param expectedValueType - Expected runtime type of the value
   * @returns The typed config value, or `undefined` if not found
   */
  #getOfType<TType extends ConfigValueType>(
    keyPath: string[],
    expectedValueType: TType,
  ): ConfigValueTypeMap[TType] | undefined {
    const keyJoined = EnvConfig.#joinKeyPath(keyPath)
    return this.#getInternal(keyPath, keyJoined, expectedValueType)
  }

  /**
   * Returns a typed config value from any source.
   *
   * Logs an error and exits if the value is missing.
   *
   * @param keyPath - Array of keys forming the config path
   * @param expectedValueType - Expected runtime type of the value
   * @returns The typed config value
   */
  #requireOfType<TType extends ConfigValueType>(
    keyPath: string[],
    expectedValueType: TType,
  ): ConfigValueTypeMap[TType] {
    const value = this.#getOfType(keyPath, expectedValueType)
    if (value === undefined) {
      Log.error(
        `Required config value ${EnvConfig.#joinKeyPath(keyPath)} is not set`,
      )
      process.exit(1)
    }
    return value as ConfigValueTypeMap[TType]
  }

  /**
   * Retrieves a config value from the supported sources in priority order.
   *
   * Values from `.env` take precedence over values from `package.json`.
   *
   * @param keyPath - Array of keys forming the config path
   * @param keyJoined - Underscore-joined key used for `.env` lookups
   * @param expectedValueType - Expected runtime type of the value
   * @returns The config value, or `undefined` if not found
   */
  #getInternal(
    keyPath: string[],
    keyJoined: string,
    expectedValueType: ConfigValueType,
  ): any {
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
  }

  /**
   * Retrieves a value from package.json "config" value.
   *
   * @param keyPath - Array of keys forming the path to the config value (e.g.,
   * ['projects', 'chrome', 'tag'])
   * @param expectedValueType - Expected type of the value
   * @returns The config value, or undefined if not found
   */
  #getPackageConfig(
    keyPath: string[],
    expectedValueType: ConfigValueType,
  ): any {
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
   * @param keyJoined - The joined key path (e.g., 'projects_chrome_tag')
   * @param expectedValueType - Expected type of the value
   * @returns The parsed configuration value, or undefined if not found
   */
  #getDotenvConfig(keyJoined: string, expectedValueType: ConfigValueType): any {
    const dotenvConfigValue = this.#dotenvConfig[keyJoined]
    if (dotenvConfigValue === undefined) {
      return undefined
    }

    // Parse as JSON or return a string if no default value is provided.
    if (expectedValueType === 'Any') {
      return EnvConfig.#parseJsonOrKeepString(dotenvConfigValue)
    }

    // Use the value as is if the expected value type is a string.
    if (expectedValueType === 'String') {
      return dotenvConfigValue
    }

    // Parse as JSON if the expected value type is not a string.
    let dotenvConfigValueParsed: any
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
   * @param configDir - Directory containing package.json
   * @returns The parsed package.json
   */
  static #loadPackageJson(configDir: string): Record<string, any> {
    const packageJsonPath = path.join(configDir, 'package.json')
    const packageJsonContent = fs.readFileSync(packageJsonPath, 'utf8')
    const packageJson: Record<string, any> = JSON.parse(packageJsonContent)
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
   * @param configDir - Directory containing .env file
   * @returns The parsed .env file
   */
  static #loadDotenvConfig(configDir: string): NodeJS.Dict<string> {
    let dotenvConfig: NodeJS.Dict<string> = {}
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
   * @param envPath - Path to the main .env file to parse
   * @returns The parsed .env file
   */
  static #parseEnvFileWithIncludes(envPath: string): NodeJS.Dict<string> {
    const seenFiles = new Set<string>()
    function readEnvFile(filePath: string, fromFile: string): string {
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

      // Trim to remove BOM.
      const envContent = fs.readFileSync(filePath, 'utf8').trim()
      const lines = envContent.split('\n')
      let result = ''

      lines.forEach((line) => {
        const includeEnvMatch = line.match(/^include_env=([^#]+)(?:#.*)?$/)
        if (includeEnvMatch && includeEnvMatch[1]) {
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
   * Validates the value against the expected value type.
   *
   * @param value - The value to validate
   * @param expectedValueType - Expected type of the value
   * @param valueDescCallback - Callback to get the
   * description of the value
   */
  static #validateValueType(
    value: any,
    expectedValueType: ConfigValueType,
    valueDescCallback: () => string,
  ) {
    if (expectedValueType === 'Any') {
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
   * @param value - Value to get the type of
   * @returns Type name
   */
  static #getValueType(value: any): ConfigValueType {
    if (value === undefined) {
      return 'Any'
    }
    if (value === null) {
      throw new Error('Null values are not allowed')
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
   * @param value - The value to parse
   * @returns The parsed value or the original value if it is not JSON-parseable
   */
  static #parseJsonOrKeepString(value: any): any {
    try {
      return JSON.parse(value)
    } catch (e) {
      return value
    }
  }

  /**
   * Joins a key path with underscores.
   *
   * @param keyPath - Array of keys forming the config path
   * @returns The joined key path
   */
  static #joinKeyPath(keyPath: string[]): string {
    assert.notEqual(keyPath.length, 0, 'keyPath must not be empty')
    return keyPath.join('_')
  }
}

type ConfigValueTypeMap = {
  Boolean: boolean
  Number: number
  String: string
  Array: any[]
  Object: Record<string, any>
  Any: any
}

type ConfigValueType = keyof ConfigValueTypeMap
