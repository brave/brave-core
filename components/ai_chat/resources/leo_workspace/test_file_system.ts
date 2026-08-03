// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// An in-memory fake of the slice of the File System Access API that the Leo
// workspace file tools use. jsdom does not implement the API at all, so the
// tests need a stand-in.
//
// The fake deliberately reproduces the parts of the spec the production code
// relies on for path confinement:
// - getFileHandle()/getDirectoryHandle() take a *single* path component and
//   reject '', '.', '..' and anything containing a path separator, so a
//   relative path can never traverse above the root handle.
// - a missing entry throws unless `create` is passed, and asking for the wrong
//   kind of entry throws.
// - createWritable() truncates: the file ends up with exactly what was written.
//
// Entries are iterated in insertion order (the real API makes no ordering
// guarantee), which lets tests distinguish code that sorts from code that
// doesn't.

// Thrown for an invalid single path component, matching the spec's TypeError.
function validateName(name: string) {
  if (name === '' || name === '.' || name === '..') {
    throw new TypeError(`Name is not allowed: ${name}`)
  }
  if (name.includes('/') || name.includes('\\')) {
    throw new TypeError(`Name contains a path separator: ${name}`)
  }
}

class FakeFileHandle {
  readonly kind = 'file'

  // `contents` is public so tests and the directory walker can read it without
  // going through the async API.
  constructor(
    readonly name: string,
    public contents: string,
  ) {}

  async getFile() {
    return {
      name: this.name,
      size: this.contents.length,
      text: async () => this.contents,
    }
  }

  async createWritable() {
    // Matches the default { keepExistingData: false }: the file is truncated
    // and ends up with whatever was written before close().
    let buffer = ''
    return {
      write: async (data: string) => {
        buffer += data
      },
      close: async () => {
        this.contents = buffer
      },
    }
  }
}

class FakeDirectoryHandle {
  readonly kind = 'directory'
  readonly children = new Map<string, FakeDirectoryHandle | FakeFileHandle>()

  constructor(readonly name: string) {}

  async getDirectoryHandle(name: string, options?: { create?: boolean }) {
    validateName(name)
    const existing = this.children.get(name)
    if (existing) {
      if (existing.kind !== 'directory') {
        throw new Error(`TypeMismatchError: ${name} is a file`)
      }
      return existing
    }
    if (!options?.create) {
      throw new Error(`NotFoundError: ${name}`)
    }
    const dir = new FakeDirectoryHandle(name)
    this.children.set(name, dir)
    return dir
  }

  async getFileHandle(name: string, options?: { create?: boolean }) {
    validateName(name)
    const existing = this.children.get(name)
    if (existing) {
      if (existing.kind !== 'file') {
        throw new Error(`TypeMismatchError: ${name} is a directory`)
      }
      return existing
    }
    if (!options?.create) {
      throw new Error(`NotFoundError: ${name}`)
    }
    const file = new FakeFileHandle(name, '')
    this.children.set(name, file)
    return file
  }

  async *entries(): AsyncGenerator<
    [string, FakeDirectoryHandle | FakeFileHandle]
  > {
    // Snapshot so that mutating the directory mid-iteration can't break tests.
    for (const entry of [...this.children.entries()]) {
      yield entry
    }
  }
}

/**
 * Builds a fake workspace root. Keys of `layout` are '/'-separated paths
 * relative to the root; intermediate directories are created automatically. A
 * key ending in '/' creates an empty directory instead of a file. Insertion
 * order is preserved when the directory is iterated.
 */
export function createFakeWorkspace(
  layout: Record<string, string> = {},
  name = 'workspace',
): FileSystemDirectoryHandle {
  const root = new FakeDirectoryHandle(name)
  for (const [path, contents] of Object.entries(layout)) {
    const isDir = path.endsWith('/')
    const parts = path.split('/').filter((p) => p !== '')
    let dir = root
    const leaf = isDir ? undefined : parts.pop()
    for (const part of parts) {
      validateName(part)
      let child = dir.children.get(part)
      if (!child) {
        child = new FakeDirectoryHandle(part)
        dir.children.set(part, child)
      }
      if (child.kind !== 'directory') {
        throw new Error(`cannot create ${path}: ${part} is a file`)
      }
      dir = child
    }
    if (leaf !== undefined) {
      validateName(leaf)
      dir.children.set(leaf, new FakeFileHandle(leaf, contents))
    }
  }
  return root as unknown as FileSystemDirectoryHandle
}

/**
 * Makes reading `path` fail, standing in for a file the picked folder lists but
 * the renderer cannot read (e.g. a permission or I/O error). The file still
 * shows up in directory listings.
 */
export function makeFileUnreadable(
  root: FileSystemDirectoryHandle,
  path: string,
) {
  const parts = path.split('/').filter((p) => p !== '')
  const name = parts.pop()
  let dir = root as unknown as FakeDirectoryHandle
  for (const part of parts) {
    const child = dir.children.get(part)
    if (child?.kind !== 'directory') {
      throw new Error(`no such directory: ${part} in ${path}`)
    }
    dir = child
  }
  const file = name === undefined ? undefined : dir.children.get(name)
  if (file?.kind !== 'file') {
    throw new Error(`no such file: ${path}`)
  }
  file.getFile = async () => {
    throw new Error(`NotAllowedError: ${path}`)
  }
}

/**
 * Flattens a fake workspace to a { path: contents } map of every file, so tests
 * can assert on the whole tree after a mutating operation. Directories are not
 * included.
 */
export function snapshotFakeWorkspace(
  root: FileSystemDirectoryHandle,
): Record<string, string> {
  const files: Record<string, string> = {}
  const walk = (dir: FakeDirectoryHandle, prefix: string) => {
    for (const [name, child] of dir.children) {
      const path = prefix ? `${prefix}/${name}` : name
      if (child.kind === 'directory') {
        walk(child, path)
      } else {
        files[path] = child.contents
      }
    }
  }
  walk(root as unknown as FakeDirectoryHandle, '')
  return files
}
