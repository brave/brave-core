// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Argument, Command } from 'commander'
import { getPassthroughArgs } from './commandsUtils.ts'

function parsePassthroughArgs(argv: string[]): string[] {
  const command = new Command()
    .name('test')
    .exitOverride()
    .argument('<suite>', 'test suite to run')
    .addArgument(
      new Argument('[build_config]', 'build configuration').argParser(
        (value) => (value.startsWith('-') ? undefined : value),
      ),
    )
    .option('--target_os <target_os>', 'target OS')
    .option('--filter <filter>', 'set test filter')
    .allowUnknownOption(true)
    .allowExcessArguments(true)
    .action(() => {})

  command.parse(argv, { from: 'user' })
  return getPassthroughArgs(command)
}

describe('getPassthroughArgs', () => {
  it.each([
    {
      name: 'suite only',
      argv: ['brave_unit_tests'],
      expected: [],
    },
    {
      name: 'suite and build_config',
      argv: ['brave_unit_tests', 'Debug'],
      expected: [],
    },
    {
      name: 'suite with known options only',
      argv: ['brave_unit_tests', '--target_os', 'win', '--filter', 'Foo.*'],
      expected: [],
    },
    {
      name: 'unknown option after suite (skipped build_config)',
      argv: ['brave_unit_tests', '--gtest_filter=Foo'],
      expected: ['--gtest_filter=Foo'],
    },
    {
      name: 'build_config then unknown options',
      argv: [
        'brave_unit_tests',
        'Debug',
        '--gtest_repeat=2',
        '--gtest_filter=Foo.*',
      ],
      expected: ['--gtest_repeat=2', '--gtest_filter=Foo.*'],
    },
    {
      name: 'known option, then unknown option',
      argv: ['brave_unit_tests', '--filter', 'Foo', '--gtest_repeat=2'],
      expected: ['--gtest_repeat=2'],
    },
    {
      name: 'mixed unknown options and excess args around skipped build_config',
      argv: [
        'abc',
        '--target_os',
        'win',
        '--gtest_filter=abc',
        'test',
        '-abc',
        'def',
        '--t',
      ],
      expected: ['--gtest_filter=abc', 'test', '-abc', 'def', '--t'],
    },
    {
      name: 'excess positional args after build_config',
      argv: ['brave_unit_tests', 'Release', 'extra', 'args'],
      expected: ['extra', 'args'],
    },
  ])('$name', ({ argv, expected }) => {
    expect(parsePassthroughArgs(argv)).toEqual(expected)
  })
})
