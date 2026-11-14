// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO(http://brave.dev/b/56449): Experimental implementation for integration
// with our bots generator.

import fs from 'node:fs'
import path from 'node:path'
import type { Command } from 'commander'
import { isCI } from './ciDetect.ts'
import config from './config.ts'
import * as Log from './log.ts'
import rootDir from './rootDir.cjs'
import type { BuildTarget, GclientTargets } from './syncTypes.ts'

export const buildersDir = path.join(
  rootDir,
  'src',
  'brave',
  'infra',
  'config',
  'generated',
  'builders',
)

// Builder names double as directory names, so keep them kebab-case.
const BUILDER_NAME_RE = /^[a-z0-9]+(-[a-z0-9]+)*$/

// A builder's spec: what it builds and syncs for, as bots declares it. Its GN
// args stay private to `bots gen`.
export interface BuilderSpec {
  builder_group: string
  chromium_config: {
    target_platform: string
    host_platform?: string | undefined
    build_config?: string | undefined
    target_arch?: string | undefined
    target_bits?: number | undefined
  }
  // What the checkout syncs besides the host, which gclient always syncs.
  gclient_config: { target_os: string[]; target_cpu: string[] }
}

// A builder's `properties.json`: the input properties of its builds, with its
// spec in the builder database of the `brave_tests_builder_config` recipe
// module's properties, as upstream's `chromium_tests_builder_config` has it.
interface BuilderProperties {
  '$brave_tests_builder_config': {
    builder_config: {
      builder_db: {
        entries: {
          builder_id: { builder: string }
          builder_spec: BuilderSpec
        }[]
      }
    }
  }
}

export interface Builder {
  name: string
  spec: BuilderSpec
  // TODO(http://brave.dev/b/56449): GN's `target_os` and `target_cpu`, the
  // only values read from the builder's `gn-args.json`, until `init`/`sync`
  // no longer need a target to build for.
  buildTarget: { targetOS: string; targetArch: string }
  targets: { compile: string[]; tests: string[] }
}

function readJson<T>(builderDir: string, file: string): T {
  const filePath = path.join(builderDir, file)
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf8'))
  } catch (e) {
    throw new Error(`Failed to read ${filePath}: ${(e as Error).message}`)
  }
}

// Loads the generated configuration of a builder (see infra/bots).
export function loadBuilder(name: string, dir = buildersDir): Builder {
  if (!BUILDER_NAME_RE.test(name)) {
    throw new Error(`invalid builder name: ${name}`)
  }
  const builderDir = path.join(dir, name)
  if (!fs.existsSync(builderDir)) {
    const available = fs.existsSync(dir) ? fs.readdirSync(dir).sort() : []
    const list = available.map((builder) => `  ${builder}`).join('\n')
    throw new Error(
      `unknown builder: ${name}`
        + (list ? `\n\navailable builders:\n${list}` : ''),
    )
  }
  const { gn_args: gnArgs } = readJson<{ gn_args: Record<string, unknown> }>(
    builderDir,
    'gn-args.json',
  )
  return {
    name,
    spec: readBuilderSpec(
      name,
      readJson<BuilderProperties>(builderDir, 'properties.json'),
    ),
    buildTarget: {
      targetOS: readGnTarget(name, gnArgs, 'target_os'),
      targetArch: readGnTarget(name, gnArgs, 'target_cpu'),
    },
    targets: readJson<Builder['targets']>(builderDir, 'targets.json'),
  }
}

function readBuilderSpec(
  name: string,
  properties: BuilderProperties,
): BuilderSpec {
  const entry = properties[
    '$brave_tests_builder_config'
  ]?.builder_config?.builder_db?.entries?.find(
    ({ builder_id: builderId }) => builderId?.builder === name,
  )
  if (!entry) {
    throw new Error(`builder ${name}: properties.json has no spec for it`)
  }
  return entry.builder_spec
}

function readGnTarget(
  name: string,
  gnArgs: Record<string, unknown>,
  field: 'target_os' | 'target_cpu',
): string {
  const value = gnArgs?.[field]
  if (typeof value !== 'string') {
    throw new Error(`builder ${name}: gn-args.json has no ${field}`)
  }
  return value
}

// Returns the `target_os` and `target_cpu` lists `init`/`sync` write into
// `.gclient` for a builder: what its checkout syncs besides the host.
export function getBuilderGclientTargets(builder: Builder): GclientTargets {
  const { target_os: targetOSList, target_cpu: targetArchList } =
    builder.spec.gclient_config
  return { targetOSList, targetArchList }
}

// Returns the one platform a builder builds for.
export function getBuilderBuildTarget(builder: Builder): BuildTarget {
  return { ...builder.buildTarget }
}

// Returns the flags and positional arguments given on the command line, other
// than `--builder` itself and `allowed` option attribute names.
export function findBuilderConflicts(
  command: Command<any, any, any>,
  allowed: string[] = [],
): string[] {
  const conflicts: string[] = []
  for (const option of command.options) {
    const key = option.attributeName()
    if (
      key !== 'builder'
      && !allowed.includes(key)
      && command.getOptionValueSource(key) === 'cli'
    ) {
      conflicts.push(option.long ?? option.short!)
    }
  }
  conflicts.push(...command.args)
  return conflicts
}

// Adds `--builder <name>`. The builder config is authoritative, so when it is
// given nothing else may be passed, except the `allowed` option attribute
// names (e.g. the `--init` that `pnpm run init` implies). `dir` is where the
// builders are looked up. The action receives the loaded `Builder` as the
// `builder` option, and `config` reads .env only as `--builder` allows.
export function supportBuilder<
  Args extends any[],
  Opts extends {},
  GlobalOpts extends {},
>(
  command: Command<Args, Opts, GlobalOpts>,
  {
    allowed = [],
    dir = buildersDir,
  }: { allowed?: string[]; dir?: string } = {},
) {
  return command
    .option(
      '--builder <name>',
      'configure everything from a builder in infra/config; no other flags or arguments are accepted',
    )
    .hook('preAction', (thisCommand) => {
      const name = thisCommand.getOptionValue('builder')
      if (typeof name !== 'string') {
        return
      }
      if (isCI) {
        thisCommand.error(
          'error: --builder is experimental and not allowed on CI',
        )
      }
      Log.warn('--builder is experimental and may change without notice.')
      const conflicts = findBuilderConflicts(thisCommand, allowed)
      if (conflicts.length) {
        thisCommand.error(
          `error: --builder cannot be combined with: ${conflicts.join(' ')}`,
        )
      }
      try {
        thisCommand.setOptionValue('builder', loadBuilder(name, dir))
      } catch (e) {
        thisCommand.error(`error: ${(e as Error).message}`)
      }
      config.useBuilder()
    })
}
