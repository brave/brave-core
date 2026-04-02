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
   * Returns the value if it exists and has the expected type; otherwise returns
   * `defaultValue` or `undefined` if missing, and errors on type mismatch.
   */
  getBoolean(keyPath: string[]): boolean | undefined
  getBoolean(keyPath: string[], defaultValue: boolean): boolean
  getBoolean(keyPath: string[], defaultValue?: boolean): boolean | undefined {
    return this.#getOfType(keyPath, 'Boolean') ?? defaultValue
  }

  /**
   * Returns a required boolean config value, errors if the value is missing.
   */
  requireBoolean(keyPath: string[]): boolean {
    return this.#requireValue(keyPath, this.getBoolean(keyPath))
  }

  /**
   * Returns the value if it exists and has the expected type; otherwise returns
   * `defaultValue` or `undefined` if missing, and errors on type mismatch.
   */
  getNumber(keyPath: string[]): number | undefined
  getNumber(keyPath: string[], defaultValue: number): number
  getNumber(keyPath: string[], defaultValue?: number): number | undefined {
    return this.#getOfType(keyPath, 'Number') ?? defaultValue
  }

  /**
   * Returns a required number config value, errors if the value is missing.
   */
  requireNumber(keyPath: string[]): number {
    return this.#requireValue(keyPath, this.getNumber(keyPath))
  }

  /**
   * Returns the value if it exists and has the expected type; otherwise returns
   * `defaultValue` or `undefined` if missing, and errors on type mismatch.
   */
  getString(keyPath: string[]): string | undefined
  getString(keyPath: string[], defaultValue: string): string
  getString(keyPath: string[], defaultValue?: string): string | undefined {
    return this.#getOfType(keyPath, 'String') ?? defaultValue
  }

  /**
   * Returns a required string config value, errors if the value is missing.
   */
  requireString(keyPath: string[]): string {
    return this.#requireValue(keyPath, this.getString(keyPath))
  }

  /**
   * Returns the value if it exists and has the expected type; otherwise returns
   * `defaultValue` or `undefined` if missing, and errors on type mismatch.
   */
  getArray(keyPath: string[]): any[] | undefined
  getArray(keyPath: string[], defaultValue: any[]): any[]
  getArray(keyPath: string[], defaultValue?: any[]): any[] | undefined {
    return this.#getOfType(keyPath, 'Array') ?? defaultValue
  }

  /**
   * Returns a required array config value, errors if the value is missing.
   */
  requireArray(keyPath: string[]): any[] {
    return this.#requireValue(keyPath, this.getArray(keyPath))
  }

  /**
   * Returns the value if it exists and has the expected type; otherwise returns
   * `defaultValue` or `undefined` if missing, and errors on type mismatch.
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
   * Returns a required object config value, errors if the value is missing.
   */
  requireObject(keyPath: string[]): Record<string, any> {
    return this.#requireValue(keyPath, this.getObject(keyPath))
  }

  /**
   * Returns a config value of any type (parsed as JSON if possible, otherwise
   * returned as a string) if present.
   */
  getAny(keyPath: string[]): any {
    return this.#getOfType(keyPath, 'Any')
  }

  /**
   * Returns a merged object from .env files and package.json.
   */
  getMergedObject(keyPath: string[]): Record<string, any> {
    const keyJoined = EnvConfig.#joinKeyPath(keyPath)

    const dotenvConfigValue = EnvConfig.#convertToValueType(
      this.#get(keyPath, 'dotenv') ?? {},
      'Object',
      keyPath,
    )
    const packageConfigValue = EnvConfig.#convertToValueType(
      this.#get(keyPath, 'package') ?? {},
      'Object',
      keyPath,
    )
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
   * Returns a required absolute path from a configuration value, errors if the
   * value is missing. Relative paths are resolved relative to the *initial
   * config directory*. Paths starting with `~` are expanded to the user's home
   * directory.
   *
   * Values from `include_env` configs are resolved relative to the same
   * *initial config directory*, not the included file's location.
   */
  requirePath(keyPath: string[]): string {
    return this.#requireValue(keyPath, this.getPath(keyPath))
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
   */
  #getOfType(keyPath: string[], expectedValueType: ConfigValueType): any {
    const value = this.#get(keyPath)
    if (value !== undefined) {
      return EnvConfig.#convertToValueType(value, expectedValueType, keyPath)
    }
  }

  /**
   * Returns a value if present, errors if the value is missing.
   */
  #requireValue<T>(keyPath: string[], value: T | undefined): T {
    if (value !== undefined) {
      return value
    }
    Log.error(
      `Required config value ${EnvConfig.#joinKeyPath(keyPath)} is not set`,
    )
    process.exit(1)
  }

  /**
   * Returns a typed config value if present in any source.
   */
  #get(keyPath: string[], source: 'dotenv' | 'package' | 'all' = 'all'): any {
    const keyJoined = EnvConfig.#joinKeyPath(keyPath)
    switch (source) {
      case 'dotenv':
        return this.#dotenvConfig[keyJoined]
      case 'package':
        return keyPath.reduce(
          (obj, subkey) => obj?.[subkey],
          this.#packageJson.config,
        )
      case 'all':
        return (
          this.#dotenvConfig[keyJoined]
          ?? keyPath.reduce(
            (obj, subkey) => obj?.[subkey],
            this.#packageJson.config,
          )
        )
      default:
        Log.error(`Invalid source: ${source}`)
        process.exit(1)
    }
  }

  /**
   * Converts a value to the expected value type.
   */
  static #convertToValueType(
    value: any,
    expectedValueType: ConfigValueType,
    keyPath: string[],
  ): any {
    if (EnvConfig.#getValueType(value) === expectedValueType) {
      return value
    }

    if (expectedValueType === 'Any') {
      return EnvConfig.#parseJsonOrKeepString(value)
    }

    try {
      const parsedValue = JSON.parse(value)
      EnvConfig.#validateValueType(parsedValue, expectedValueType, () =>
        EnvConfig.#joinKeyPath(keyPath),
      )
      return parsedValue
    } catch (e) {
      Log.error(
        `${EnvConfig.#joinKeyPath(keyPath)} value is not JSON-parseable:\n${e.message}`,
      )
      process.exit(1)
    }
  }

  /**
   * Loads package.json file from the specified directory.
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
   * Returns a string representing the type of a value.
   */
  static #getValueType(value: any): ConfigValueType {
    if (value === undefined || value === null) {
      return 'Any'
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
   * Parses a value as JSON or returns it as is if it is not JSON-parseable.
   */
  static #parseJsonOrKeepString(value: any): any {
    if (value === undefined) {
      return value
    }

    try {
      return JSON.parse(value)
    } catch (e) {
      return value
    }
  }

  /**
   * Joins a key path with underscores, errors if the key path is empty.
   */
  static #joinKeyPath(keyPath: string[]): string {
    assert.notEqual(keyPath.length, 0, 'keyPath must not be empty')
    const joinedKeyPath = keyPath.join('_')
    assert.notEqual(joinedKeyPath, '', 'joinedKeyPath must not be empty')
    return joinedKeyPath
  }
}

type ConfigValueType =
  | 'Boolean'
  | 'Number'
  | 'String'
  | 'Array'
  | 'Object'
  | 'Any'
