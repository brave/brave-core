// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO(http://brave.dev/b/56449): Experimental implementation for integration
// with our bots generator.

import fs from 'node:fs'
import path from 'node:path'
import type { BuilderSpec } from './builderTypes.ts'

// The spec of the builders writeMockBuilder() writes.
export const mockBuilderSpec: BuilderSpec = {
  builder_group: 'brave.test',
  chromium_config: { target_platform: 'linux', build_config: 'Release' },
  gclient_config: { target_os: ['android'], target_cpu: [] },
  brave_config: {
    is_brave_release_build: false,
    ignore_patch_version_number: false,
    use_brave_hermetic_toolchain: false,
  },
}

// A `properties.json` with `mockBuilderSpec` as `name`'s.
export function mockBuilderProperties(name: string): object {
  return {
    $brave_tests_builder_config: {
      builder_config: {
        builder_db: {
          entries: [
            { builder_id: { builder: name }, builder_spec: mockBuilderSpec },
          ],
        },
        builder_ids: [{ builder: name }],
      },
    },
    builder_group: 'brave.test',
    recipe: 'brave_builder',
  }
}

/**
 * Writes the generated files of a Linux x64 builder `name` into `dir`, for
 * tests, as `bots.py snapshot` writes them into infra/config/generated.
 *
 * @param dir - The builders directory
 * @param name - The builder's name
 */
export function writeMockBuilder(dir: string, name: string): void {
  const builderDir = path.join(dir, name)
  fs.mkdirSync(builderDir, { recursive: true })
  fs.writeFileSync(
    path.join(builderDir, 'properties.json'),
    JSON.stringify(mockBuilderProperties(name)),
  )
  fs.writeFileSync(
    path.join(builderDir, 'gn-args.json'),
    JSON.stringify({
      gn_args: { target_os: 'linux', target_cpu: 'x64', is_asan: true },
      secrets: { service_key: 'SERVICE_KEY', api_key: 'API_KEY' },
    }),
  )
  fs.writeFileSync(
    path.join(builderDir, 'targets.json'),
    JSON.stringify({ compile: ['brave:all'], tests: ['brave_unit_tests'] }),
  )
}
