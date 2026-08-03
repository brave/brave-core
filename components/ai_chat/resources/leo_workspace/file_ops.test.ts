// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as ops from './file_ops'
import {
  createFakeWorkspace,
  makeFileUnreadable,
  snapshotFakeWorkspace,
} from './test_file_system'

// Sorts the lines of a tool result. grep/glob walk directories in whatever
// order the platform hands entries back, so only the set of lines is
// meaningful.
function sortedLines(result: string): string[] {
  return result.split('\n').sort()
}

describe('isDirectory', () => {
  it('treats the empty path as the workspace root', async () => {
    const root = createFakeWorkspace({ 'a.txt': 'a' })
    expect(await ops.isDirectory(root, '')).toBe(true)
  })

  it('is true for a directory, with or without a trailing slash', async () => {
    const root = createFakeWorkspace({ 'src/a.ts': 'a' })
    expect(await ops.isDirectory(root, 'src')).toBe(true)
    expect(await ops.isDirectory(root, 'src/')).toBe(true)
    expect(await ops.isDirectory(root, '/src')).toBe(true)
  })

  it('is false for a file and for a path that does not exist', async () => {
    const root = createFakeWorkspace({ 'src/a.ts': 'a' })
    expect(await ops.isDirectory(root, 'src/a.ts')).toBe(false)
    expect(await ops.isDirectory(root, 'nope')).toBe(false)
    expect(await ops.isDirectory(root, 'src/nope/deeper')).toBe(false)
  })

  it('is false rather than throwing for an escaping path', async () => {
    const root = createFakeWorkspace({ 'src/a.ts': 'a' })
    expect(await ops.isDirectory(root, '..')).toBe(false)
    expect(await ops.isDirectory(root, 'src/../..')).toBe(false)
  })
})

describe('listDir', () => {
  // Deliberately not in alphabetical order, to prove listDir sorts.
  const layout = {
    'src/index.ts': '',
    'src/a.ts': '',
    'src/nested/deep/deeper.ts': '',
    'readme.md': '',
    'docs/': '',
  }

  it('sorts entries and marks directories with a trailing slash', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.listDir(root, '', 2)).toBe(
      [
        'docs/',
        'readme.md',
        'src/',
        'src/a.ts',
        'src/index.ts',
        'src/nested/',
      ].join('\n'),
    )
  })

  it('sorts case-insensitively rather than by code unit', async () => {
    const root = createFakeWorkspace({ 'B.txt': '', 'a.txt': '' })
    expect(await ops.listDir(root, '', 2)).toBe('a.txt\nB.txt')
  })

  it('defaults to a depth of two levels', async () => {
    const root = createFakeWorkspace(layout)
    // 0 and negative depths fall back to the default, which is the same as an
    // explicit depth of 2.
    const expected = await ops.listDir(root, '', 2)
    expect(await ops.listDir(root, '', 0)).toBe(expected)
    expect(await ops.listDir(root, '', -1)).toBe(expected)
  })

  it('descends further when asked', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.listDir(root, '', 3)).toBe(
      [
        'docs/',
        'readme.md',
        'src/',
        'src/a.ts',
        'src/index.ts',
        'src/nested/',
        'src/nested/deep/',
      ].join('\n'),
    )
    expect(await ops.listDir(root, '', 4)).toContain(
      'src/nested/deep/deeper.ts',
    )
  })

  it('lists a subdirectory with paths relative to the root', async () => {
    const root = createFakeWorkspace(layout)
    // The depth is counted from the listed directory, not from the root, so
    // two levels below src/ are shown.
    const expected = [
      'src/a.ts',
      'src/index.ts',
      'src/nested/',
      'src/nested/deep/',
    ].join('\n')
    expect(await ops.listDir(root, 'src', 2)).toBe(expected)
    // Leading and trailing slashes must not leak into the output.
    expect(await ops.listDir(root, '/src/', 2)).toBe(expected)
  })

  it('reports an empty directory', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.listDir(root, 'docs', 2)).toBe('(empty)')
    expect(await ops.listDir(createFakeWorkspace(), '', 2)).toBe('(empty)')
  })

  it('rejects a path that does not exist', async () => {
    const root = createFakeWorkspace(layout)
    await expect(ops.listDir(root, 'nope', 2)).rejects.toThrow()
  })
})

describe('viewFile', () => {
  const layout = { 'lines.txt': 'one\ntwo\nthree\nfour\nfive' }

  it('numbers every line from 1 when no range is given', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.viewFile(root, 'lines.txt', undefined)).toBe(
      '1\tone\n2\ttwo\n3\tthree\n4\tfour\n5\tfive',
    )
  })

  it('counts a trailing newline as a final empty line', async () => {
    const root = createFakeWorkspace({ 'a.txt': 'one\n' })
    expect(await ops.viewFile(root, 'a.txt', undefined)).toBe('1\tone\n2\t')
  })

  it('shows only the requested range', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.viewFile(root, 'lines.txt', [2, 4])).toBe(
      '2\ttwo\n3\tthree\n4\tfour',
    )
  })

  it('reads to the end of the file for an end of -1', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.viewFile(root, 'lines.txt', [3, -1])).toBe(
      '3\tthree\n4\tfour\n5\tfive',
    )
  })

  it('reads to the end of the file when only a start is given', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.viewFile(root, 'lines.txt', [4])).toBe('4\tfour\n5\tfive')
  })

  it('clamps a range that runs outside the file', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.viewFile(root, 'lines.txt', [0, 2])).toBe('1\tone\n2\ttwo')
    expect(await ops.viewFile(root, 'lines.txt', [4, 99])).toBe(
      '4\tfour\n5\tfive',
    )
  })

  it('returns nothing for a range past the end of the file', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.viewFile(root, 'lines.txt', [10, 20])).toBe('')
  })

  it('handles an empty file', async () => {
    const root = createFakeWorkspace({ 'empty.txt': '' })
    expect(await ops.viewFile(root, 'empty.txt', undefined)).toBe('1\t')
  })

  it('rejects a file that does not exist', async () => {
    const root = createFakeWorkspace(layout)
    await expect(ops.viewFile(root, 'nope.txt', undefined)).rejects.toThrow()
  })
})

describe('grep', () => {
  const layout = {
    'a.txt': 'alpha\nbeta\ngamma\n',
    'sub/b.txt': 'beta\ndelta\n',
    'sub/c.ts': 'const beta = 1\n',
  }

  it('reports path, 1-indexed line number and the matching line', async () => {
    const root = createFakeWorkspace(layout)
    expect(sortedLines(await ops.grep(root, '', 'beta', ''))).toEqual([
      'a.txt:2: beta',
      'sub/b.txt:1: beta',
      'sub/c.ts:1: const beta = 1',
    ])
  })

  it('treats the pattern as a regular expression', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.grep(root, '', '^beta$', '')).toBe(
      ['a.txt:2: beta', 'sub/b.txt:1: beta'].sort().join('\n'),
    )
    expect(await ops.grep(root, '', 'a(lpha|mma)', '')).toBe(
      ['a.txt:1: alpha', 'a.txt:3: gamma'].join('\n'),
    )
  })

  it('searches only under the given directory', async () => {
    const root = createFakeWorkspace(layout)
    expect(sortedLines(await ops.grep(root, 'sub', 'beta', ''))).toEqual([
      'sub/b.txt:1: beta',
      'sub/c.ts:1: const beta = 1',
    ])
  })

  it('filters by an include glob matched against the file name', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.grep(root, '', 'beta', '*.ts')).toBe(
      'sub/c.ts:1: const beta = 1',
    )
  })

  it('filters by an include glob matched against the relative path', async () => {
    const root = createFakeWorkspace(layout)
    expect(sortedLines(await ops.grep(root, '', 'beta', '**/*.txt'))).toEqual([
      'a.txt:2: beta',
      'sub/b.txt:1: beta',
    ])
    expect(await ops.grep(root, '', 'beta', 'sub/*.txt')).toBe(
      'sub/b.txt:1: beta',
    )
  })

  it('reports when nothing matches', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.grep(root, '', 'nothing here', '')).toBe('(no matches)')
    expect(await ops.grep(root, '', 'beta', '*.md')).toBe('(no matches)')
  })

  it('skips files it cannot read', async () => {
    const root = createFakeWorkspace(layout)
    makeFileUnreadable(root, 'sub/b.txt')
    expect(sortedLines(await ops.grep(root, '', 'beta', ''))).toEqual([
      'a.txt:2: beta',
      'sub/c.ts:1: const beta = 1',
    ])
  })

  it('caps the number of matches it returns', async () => {
    const root = createFakeWorkspace({
      'many.txt': Array(300).fill('needle').join('\n'),
    })
    const lines = (await ops.grep(root, '', 'needle', '')).split('\n')
    expect(lines).toHaveLength(201)
    expect(lines[199]).toBe('many.txt:200: needle')
    expect(lines[200]).toBe('... (truncated at 200 matches)')
  })

  it('rejects an invalid regular expression', async () => {
    const root = createFakeWorkspace(layout)
    await expect(ops.grep(root, '', '([', '')).rejects.toThrow()
  })

  it('rejects a directory that does not exist', async () => {
    const root = createFakeWorkspace(layout)
    await expect(ops.grep(root, 'nope', 'beta', '')).rejects.toThrow()
  })
})

describe('glob', () => {
  const layout = {
    'a.txt': '',
    'sub/b.txt': '',
    'sub/c.ts': '',
    'sub/deep/d.ts': '',
  }

  it('matches relative paths with **', async () => {
    const root = createFakeWorkspace(layout)
    expect(sortedLines(await ops.glob(root, '', '**/*.ts'))).toEqual([
      'sub/c.ts',
      'sub/deep/d.ts',
    ])
  })

  it('matches a bare pattern against the file name at any depth', async () => {
    const root = createFakeWorkspace(layout)
    expect(sortedLines(await ops.glob(root, '', '*.txt'))).toEqual([
      'a.txt',
      'sub/b.txt',
    ])
    expect(await ops.glob(root, '', '?.ts')).toBe(
      ['sub/c.ts', 'sub/deep/d.ts'].sort().join('\n'),
    )
  })

  it('does not let * cross a path separator', async () => {
    const root = createFakeWorkspace(layout)
    expect(sortedLines(await ops.glob(root, '', 'sub/*'))).toEqual([
      'sub/b.txt',
      'sub/c.ts',
    ])
  })

  it('escapes regex metacharacters in the pattern', async () => {
    const root = createFakeWorkspace({ 'a.txt': '', 'axtxt': '' })
    expect(await ops.glob(root, '', '*.txt')).toBe('a.txt')
  })

  it('searches only under the given directory', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.glob(root, 'sub/deep', '*.ts')).toBe('sub/deep/d.ts')
  })

  it('reports when nothing matches', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.glob(root, '', '*.md')).toBe('(no matches)')
  })

  it('caps the number of results it returns', async () => {
    const layoutWithManyFiles: Record<string, string> = {}
    for (let i = 0; i < 600; i++) {
      layoutWithManyFiles[`f${i}.txt`] = ''
    }
    const root = createFakeWorkspace(layoutWithManyFiles)
    const lines = (await ops.glob(root, '', '*.txt')).split('\n')
    expect(lines).toHaveLength(501)
    expect(lines[500]).toBe('... (truncated at 500 results)')
  })
})

describe('createFile', () => {
  it('creates a file and any missing parent directories', async () => {
    const root = createFakeWorkspace()
    expect(await ops.createFile(root, 'a/b/c.txt', 'hello')).toBe(
      'Created a/b/c.txt (5 bytes)',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a/b/c.txt': 'hello' })
  })

  it('overwrites an existing file', async () => {
    const root = createFakeWorkspace({ 'a.txt': 'old contents' })
    expect(await ops.createFile(root, 'a.txt', 'new')).toBe(
      'Overwrote a.txt (3 bytes)',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': 'new' })
  })

  it('creates an empty file', async () => {
    const root = createFakeWorkspace()
    expect(await ops.createFile(root, 'a.txt', '')).toBe(
      'Created a.txt (0 bytes)',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': '' })
  })

  it('rejects a path with no file name', async () => {
    const root = createFakeWorkspace()
    await expect(ops.createFile(root, '', 'x')).rejects.toThrow(
      'empty file path',
    )
    await expect(ops.createFile(root, '/', 'x')).rejects.toThrow(
      'empty file path',
    )
  })

  it('rejects a path that names an existing directory', async () => {
    const root = createFakeWorkspace({ 'src/a.ts': '' })
    await expect(ops.createFile(root, 'src', 'x')).rejects.toThrow()
  })
})

describe('appendFile', () => {
  it('appends to an existing file', async () => {
    const root = createFakeWorkspace({ 'a.txt': 'one\n' })
    expect(await ops.appendFile(root, 'a.txt', 'two\n')).toBe(
      'Appended 4 bytes to a.txt',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': 'one\ntwo\n' })
  })

  it('creates the file when it does not exist yet', async () => {
    const root = createFakeWorkspace()
    expect(await ops.appendFile(root, 'a/b.txt', 'hi')).toBe(
      'Appended 2 bytes to a/b.txt',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a/b.txt': 'hi' })
  })

  it('accumulates across calls, which is the documented usage', async () => {
    const root = createFakeWorkspace()
    await ops.createFile(root, 'a.txt', '')
    await ops.appendFile(root, 'a.txt', 'one\n')
    await ops.appendFile(root, 'a.txt', 'two\n')
    await ops.appendFile(root, 'a.txt', 'three')
    expect(snapshotFakeWorkspace(root)).toEqual({
      'a.txt': 'one\ntwo\nthree',
    })
  })
})

describe('strReplace', () => {
  it('replaces the single occurrence of old_str', async () => {
    const root = createFakeWorkspace({ 'a.ts': 'const x = 1\nconst y = 2\n' })
    expect(await ops.strReplace(root, 'a.ts', 'y = 2', 'y = 3')).toBe(
      'Replaced text in a.ts',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({
      'a.ts': 'const x = 1\nconst y = 3\n',
    })
  })

  it('replaces text spanning several lines', async () => {
    const root = createFakeWorkspace({ 'a.ts': 'a\nb\nc\n' })
    await ops.strReplace(root, 'a.ts', 'a\nb\n', 'x\n')
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.ts': 'x\nc\n' })
  })

  it('can replace with the empty string to delete text', async () => {
    const root = createFakeWorkspace({ 'a.ts': 'keep\ndrop\n' })
    await ops.strReplace(root, 'a.ts', 'drop\n', '')
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.ts': 'keep\n' })
  })

  it('rejects when old_str is not present', async () => {
    const root = createFakeWorkspace({ 'a.ts': 'const x = 1\n' })
    await expect(ops.strReplace(root, 'a.ts', 'missing', 'x')).rejects.toThrow(
      'old_str was not found in the file',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.ts': 'const x = 1\n' })
  })

  it('rejects when old_str appears more than once', async () => {
    const root = createFakeWorkspace({ 'a.ts': 'dup\ndup\n' })
    await expect(ops.strReplace(root, 'a.ts', 'dup', 'x')).rejects.toThrow(
      'old_str is not unique in the file',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.ts': 'dup\ndup\n' })
  })

  it('rejects overlapping occurrences of old_str', async () => {
    const root = createFakeWorkspace({ 'a.ts': 'aaa' })
    await expect(ops.strReplace(root, 'a.ts', 'aa', 'b')).rejects.toThrow(
      'old_str is not unique in the file',
    )
  })

  it('rejects an empty old_str rather than inserting at the start', async () => {
    const root = createFakeWorkspace({ 'a.ts': 'abc' })
    await expect(ops.strReplace(root, 'a.ts', '', 'x')).rejects.toThrow(
      'old_str is not unique in the file',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.ts': 'abc' })
  })

  it('rejects a file that does not exist', async () => {
    const root = createFakeWorkspace()
    await expect(ops.strReplace(root, 'nope.ts', 'a', 'b')).rejects.toThrow()
  })
})

describe('insert', () => {
  const layout = { 'a.txt': 'one\ntwo\nthree' }

  it('inserts at the start of the file for line 0', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.insert(root, 'a.txt', 0, 'zero')).toBe(
      'Inserted 1 line(s) into a.txt after line 0',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({
      'a.txt': 'zero\none\ntwo\nthree',
    })
  })

  it('inserts after the given 1-indexed line', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.insert(root, 'a.txt', 2, 'two-and-a-half')).toBe(
      'Inserted 1 line(s) into a.txt after line 2',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({
      'a.txt': 'one\ntwo\ntwo-and-a-half\nthree',
    })
  })

  it('inserts multi-line text and reports the line count', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.insert(root, 'a.txt', 1, 'x\ny')).toBe(
      'Inserted 2 line(s) into a.txt after line 1',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({
      'a.txt': 'one\nx\ny\ntwo\nthree',
    })
  })

  it('clamps a line number past the end of the file', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.insert(root, 'a.txt', 99, 'last')).toBe(
      'Inserted 1 line(s) into a.txt after line 3',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({
      'a.txt': 'one\ntwo\nthree\nlast',
    })
  })

  it('clamps a negative line number to the start of the file', async () => {
    const root = createFakeWorkspace(layout)
    expect(await ops.insert(root, 'a.txt', -5, 'first')).toBe(
      'Inserted 1 line(s) into a.txt after line 0',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({
      'a.txt': 'first\none\ntwo\nthree',
    })
  })

  it('rejects a file that does not exist', async () => {
    const root = createFakeWorkspace()
    await expect(ops.insert(root, 'nope.txt', 0, 'x')).rejects.toThrow()
  })
})

// The whole point of driving these tools through a FileSystemDirectoryHandle is
// that the workspace root is a hard boundary, so check it directly.
describe('workspace confinement', () => {
  const layout = { 'src/a.ts': 'contents', 'a.txt': 'contents' }

  it('rejects a path containing ..', async () => {
    const root = createFakeWorkspace(layout)
    const escapes = /path escapes the workspace root/
    await expect(ops.viewFile(root, '../a.txt', undefined)).rejects.toThrow(
      escapes,
    )
    await expect(ops.listDir(root, 'src/..', 2)).rejects.toThrow(escapes)
    await expect(ops.grep(root, '..', 'x', '')).rejects.toThrow(escapes)
    await expect(ops.glob(root, '..', '*')).rejects.toThrow(escapes)
    await expect(ops.createFile(root, '../evil.txt', 'x')).rejects.toThrow(
      escapes,
    )
    await expect(ops.appendFile(root, '../evil.txt', 'x')).rejects.toThrow(
      escapes,
    )
    await expect(ops.strReplace(root, '../a.txt', 'a', 'b')).rejects.toThrow(
      escapes,
    )
    await expect(ops.insert(root, '../a.txt', 0, 'x')).rejects.toThrow(escapes)
  })

  it('rejects .. in the middle of a path', async () => {
    const root = createFakeWorkspace(layout)
    await expect(
      ops.viewFile(root, 'src/../../a.txt', undefined),
    ).rejects.toThrow(/path escapes the workspace root/)
  })

  it('rejects .. that is only apparent after trimming', async () => {
    const root = createFakeWorkspace(layout)
    await expect(
      ops.viewFile(root, 'src/ .. /a.txt', undefined),
    ).rejects.toThrow(/path escapes the workspace root/)
  })

  it('ignores . segments and redundant separators', async () => {
    const root = createFakeWorkspace(layout)
    const expected = '1\tcontents'
    expect(await ops.viewFile(root, './src/./a.ts', undefined)).toBe(expected)
    expect(await ops.viewFile(root, '/src//a.ts', undefined)).toBe(expected)
    expect(await ops.viewFile(root, 'src/a.ts/', undefined)).toBe(expected)
  })

  it('leaves an absolute-looking path relative to the root', async () => {
    // A leading '/' is stripped rather than escaping to the real filesystem
    // root, so this reads the workspace's own a.txt.
    const root = createFakeWorkspace(layout)
    expect(await ops.viewFile(root, '/a.txt', undefined)).toBe('1\tcontents')
  })
})
