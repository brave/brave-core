// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
import rootDir from './rootDir.ts'
import type { PatchedRepository } from './repositories.ts'
import {
  getPatchedRepositories,
  getRepositoriesFilePath,
  joinSourcePath,
  parsePatchedRepositories,
  splitSourcePath,
} from './repositories.ts'

jest.mock('./config.ts', () => ({
  __esModule: true,
  default: {
    braveCoreDir: path.join('/workspace', 'src', 'brave'),
    srcDir: path.join('/workspace', 'src'),
  },
}))

const filePath = 'patches/.repositories.cfg'

// A list shaped like brave-core's own, for the resolution tests.
const repositories = (...relativePaths: string[]): PatchedRepository[] =>
  relativePaths.map((relativePath) => ({
    label: `//${relativePath}`,
    path: path.join('/src', relativePath),
    patchDir: path.join('/patches', relativePath),
    isChromium: relativePath.length === 0,
  }))

describe('parsePatchedRepositories', () => {
  it('reads gn-style source-absolute paths', () => {
    expect(
      parsePatchedRepositories('//\n//v8\n//third_party/ffmpeg\n', filePath),
    ).toEqual(['', 'v8', 'third_party/ffmpeg'])
  })

  it('keeps the order the file lists', () => {
    expect(
      parsePatchedRepositories('//v8\n//\n//third_party/ffmpeg\n', filePath),
    ).toEqual(['v8', '', 'third_party/ffmpeg'])
  })

  it('ignores comments and blank lines', () => {
    const contents = [
      '# a leading comment',
      '',
      '//  # chromium',
      '//v8',
      '#//third_party/ffmpeg',
      '   ',
    ].join('\n')
    expect(parsePatchedRepositories(contents, filePath)).toEqual(['', 'v8'])
  })

  it('tolerates a trailing slash', () => {
    expect(parsePatchedRepositories('//\n//v8/\n', filePath)).toEqual([
      '',
      'v8',
    ])
  })

  it.each([
    {
      name: 'a path with no source-root prefix',
      contents: '//\nv8\n',
      expected: /must be source-absolute/,
    },
    {
      name: 'a relative path',
      contents: '//\n./v8\n',
      expected: /must be source-absolute/,
    },
    {
      name: 'a single-slash path',
      contents: '//\n/v8\n',
      expected: /must be source-absolute/,
    },
    {
      name: 'an upward path',
      contents: '//\n//../v8\n',
      expected: /cannot traverse upwards/,
    },
    {
      name: 'a duplicated path',
      contents: '//\n//v8\n//v8\n',
      expected: /listed more than once/,
    },
    {
      name: 'a duplicated chromium',
      contents: '//\n//v8\n//\n',
      expected: /listed more than once/,
    },
    {
      name: 'a file without chromium',
      contents: '//v8\n',
      expected: /does not list Chromium/,
    },
    {
      name: 'an empty file',
      contents: '',
      expected: /does not list Chromium/,
    },
  ])('rejects $name', ({ contents, expected }) => {
    expect(() => parsePatchedRepositories(contents, filePath)).toThrow(expected)
  })

  it('names the file and line of a bad entry', () => {
    expect(() =>
      parsePatchedRepositories('//\n//v8\n//../elsewhere\n', filePath),
    ).toThrow(`${filePath}:3:`)
  })

  it('accepts brave-core’s own repositories file', () => {
    // The real file has to parse, or neither this tool nor plaster can run.
    const realPath = path.join(
      rootDir,
      'src',
      'brave',
      'patches',
      '.repositories.cfg',
    )
    const parsed = parsePatchedRepositories(
      fs.readFileSync(realPath, 'utf-8'),
      realPath,
    )
    expect(parsed).toContain('')
    expect(parsed).toContain('v8')
  })
})

describe('getPatchedRepositories', () => {
  afterEach(() => {
    jest.restoreAllMocks()
  })

  const mockFile = (contents: string) => {
    jest.spyOn(fs, 'readFileSync').mockImplementation((requested) => {
      expect(requested).toBe(getRepositoriesFilePath())
      return contents
    })
  }

  it('resolves each repository’s source and patch directories', () => {
    mockFile('//\n//v8\n//third_party/devtools-frontend/src\n')
    expect(getPatchedRepositories()).toEqual([
      {
        label: '//',
        path: path.join('/workspace', 'src'),
        patchDir: path.join('/workspace', 'src', 'brave', 'patches'),
        isChromium: true,
      },
      {
        label: '//v8',
        path: path.join('/workspace', 'src', 'v8'),
        patchDir: path.join('/workspace', 'src', 'brave', 'patches', 'v8'),
        isChromium: false,
      },
      {
        label: '//third_party/devtools-frontend/src',
        path: path.join(
          '/workspace',
          'src',
          'third_party',
          'devtools-frontend',
          'src',
        ),
        patchDir: path.join(
          '/workspace',
          'src',
          'brave',
          'patches',
          'third_party',
          'devtools-frontend',
          'src',
        ),
        isChromium: false,
      },
    ])
  })

  it('reads the file next to the patches it describes', () => {
    expect(getRepositoriesFilePath()).toBe(
      path.join('/workspace', 'src', 'brave', 'patches', '.repositories.cfg'),
    )
  })
})

describe('splitSourcePath', () => {
  const all = repositories(
    '',
    'v8',
    'third_party/devtools-frontend/src',
    'third_party/ffmpeg',
  )

  it.each([
    {
      name: 'a chromium source',
      source: 'base/memory/foo.h',
      repository: '',
      relativePath: 'base/memory/foo.h',
    },
    {
      name: 'a source in a listed repository',
      source: 'v8/src/codegen/compiler.cc',
      repository: 'v8',
      relativePath: 'src/codegen/compiler.cc',
    },
    {
      name: 'a source in a nested repository',
      source: 'third_party/devtools-frontend/src/front_end/core/Foo.ts',
      repository: 'third_party/devtools-frontend/src',
      relativePath: 'front_end/core/Foo.ts',
    },
    {
      name: 'a source under an unlisted directory',
      source: 'third_party/blink/renderer/bar.cc',
      repository: '',
      relativePath: 'third_party/blink/renderer/bar.cc',
    },
    {
      name: 'a path that merely starts like a repository',
      source: 'v8_extras/foo.cc',
      repository: '',
      relativePath: 'v8_extras/foo.cc',
    },
    {
      name: 'a repository path with no source after it',
      source: 'v8',
      repository: '',
      relativePath: 'v8',
    },
  ])('splits $name', ({ source, repository, relativePath }) => {
    const split = splitSourcePath(all, source)
    expect(split.repository.label).toBe(`//${repository}`)
    expect(split.relativePath).toBe(relativePath)
  })

  it('prefers the most specific repository regardless of order', () => {
    const unordered = repositories(
      '',
      'third_party/devtools-frontend/src',
      'third_party',
    )
    const split = splitSourcePath(
      unordered,
      'third_party/devtools-frontend/src/front_end/Foo.ts',
    )
    expect(split.repository.label).toBe('//third_party/devtools-frontend/src')
    expect(split.relativePath).toBe('front_end/Foo.ts')
  })

  it('throws when the list has no chromium entry', () => {
    expect(() => splitSourcePath(repositories('v8'), 'base/foo.h')).toThrow(
      /does not include Chromium/,
    )
  })

  it.each([
    'base/memory/foo.h',
    'v8/src/codegen/compiler.cc',
    'third_party/devtools-frontend/src/front_end/core/Foo.ts',
    'third_party/blink/renderer/bar.cc',
  ])('round-trips %s through joinSourcePath', (source) => {
    const split = splitSourcePath(all, source)
    expect(joinSourcePath(split.repository, split.relativePath)).toBe(source)
  })
})

describe('joinSourcePath', () => {
  const [chromium, v8] = repositories('', 'v8') as [
    PatchedRepository,
    PatchedRepository,
  ]

  it('leaves a chromium source as it is', () => {
    expect(joinSourcePath(chromium, 'base/memory/foo.h')).toBe(
      'base/memory/foo.h',
    )
  })

  it('qualifies a source with the repository holding it', () => {
    expect(joinSourcePath(v8, 'src/codegen/compiler.cc')).toBe(
      'v8/src/codegen/compiler.cc',
    )
  })
})
