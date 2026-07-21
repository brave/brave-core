// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// File tools for the Leo workspace, implemented against a
// FileSystemDirectoryHandle using the Web File System Access API. Path
// confinement comes from the platform: getFileHandle/getDirectoryHandle take a
// single path segment and reject '..' and separators, and the handle is scoped
// to the picked folder, so there is no way to escape the workspace root.

// Caps to keep tool output within reason for the model.
const kMaxGrepMatches = 200
const kMaxGlobResults = 500

function splitPath(rel: string): string[] {
  const parts = rel
    .split('/')
    .map((s) => s.trim())
    .filter((s) => s !== '' && s !== '.')
  for (const p of parts) {
    if (p === '..') {
      throw new Error(`path escapes the workspace root: ${rel}`)
    }
  }
  return parts
}

function basename(rel: string): string {
  const parts = splitPath(rel)
  return parts.length ? parts[parts.length - 1] : ''
}

async function getDir(
  root: FileSystemDirectoryHandle,
  rel: string,
  create = false,
): Promise<FileSystemDirectoryHandle> {
  let dir = root
  for (const seg of splitPath(rel)) {
    dir = await dir.getDirectoryHandle(seg, { create })
  }
  return dir
}

async function getFile(
  root: FileSystemDirectoryHandle,
  rel: string,
  create = false,
): Promise<FileSystemFileHandle> {
  const parts = splitPath(rel)
  if (parts.length === 0) {
    throw new Error('empty file path')
  }
  const name = parts.pop() as string
  let dir = root
  for (const seg of parts) {
    dir = await dir.getDirectoryHandle(seg, { create })
  }
  return dir.getFileHandle(name, { create })
}

async function fileExists(
  root: FileSystemDirectoryHandle,
  rel: string,
): Promise<boolean> {
  try {
    await getFile(root, rel)
    return true
  } catch {
    return false
  }
}

// Whether |rel| resolves to a directory (an empty path is the root directory).
export async function isDirectory(
  root: FileSystemDirectoryHandle,
  rel: string,
): Promise<boolean> {
  try {
    await getDir(root, rel)
    return true
  } catch {
    return false
  }
}

async function readText(
  root: FileSystemDirectoryHandle,
  rel: string,
): Promise<string> {
  const fh = await getFile(root, rel)
  return (await fh.getFile()).text()
}

async function writeText(
  root: FileSystemDirectoryHandle,
  rel: string,
  content: string,
): Promise<void> {
  const fh = await getFile(root, rel, true)
  const writable = await fh.createWritable()
  await writable.write(content)
  await writable.close()
}

// dirHandle.entries() isn't in every TS lib; access via a loose type.
function entriesOf(
  dir: FileSystemDirectoryHandle,
): AsyncIterable<[string, FileSystemHandle]> {
  return (
    dir as unknown as {
      entries(): AsyncIterable<[string, FileSystemHandle]>
    }
  ).entries()
}

function join(base: string, name: string): string {
  return base ? `${base}/${name}` : name
}

function globToRegExp(glob: string): RegExp {
  let re = ''
  for (let i = 0; i < glob.length; i++) {
    const c = glob[i]
    if (c === '*') {
      if (glob[i + 1] === '*') {
        re += '.*'
        i++
        if (glob[i + 1] === '/') {
          i++
        }
      } else {
        re += '[^/]*'
      }
    } else if (c === '?') {
      re += '[^/]'
    } else if ('.+^${}()|[]\\'.includes(c)) {
      re += '\\' + c
    } else {
      re += c
    }
  }
  return new RegExp('^' + re + '$')
}

// Recursively yields the relative path of every file under |dir|.
async function* walkFiles(
  dir: FileSystemDirectoryHandle,
  prefix = '',
): AsyncGenerator<{ path: string; handle: FileSystemFileHandle }> {
  for await (const [name, handle] of entriesOf(dir)) {
    const rel = join(prefix, name)
    if (handle.kind === 'directory') {
      yield* walkFiles(handle as FileSystemDirectoryHandle, rel)
    } else {
      yield { path: rel, handle: handle as FileSystemFileHandle }
    }
  }
}

export async function listDir(
  root: FileSystemDirectoryHandle,
  path: string,
  depth: number,
): Promise<string> {
  const start = await getDir(root, path)
  const lines: string[] = []
  const maxDepth = depth && depth > 0 ? depth : 2

  async function recurse(
    dir: FileSystemDirectoryHandle,
    rel: string,
    d: number,
  ) {
    const names: Array<[string, FileSystemHandle]> = []
    for await (const entry of entriesOf(dir)) {
      names.push(entry)
    }
    names.sort((a, b) => a[0].localeCompare(b[0]))
    for (const [name, handle] of names) {
      const childRel = join(rel, name)
      if (handle.kind === 'directory') {
        lines.push(`${childRel}/`)
        if (d < maxDepth) {
          await recurse(handle as FileSystemDirectoryHandle, childRel, d + 1)
        }
      } else {
        lines.push(childRel)
      }
    }
  }

  await recurse(start, path.replace(/\/+$/, '').replace(/^\/+/, ''), 1)
  return lines.length ? lines.join('\n') : '(empty)'
}

export async function viewFile(
  root: FileSystemDirectoryHandle,
  path: string,
  range: number[] | undefined,
): Promise<string> {
  const text = await readText(root, path)
  const lines = text.split('\n')
  let start = 1
  let end = lines.length
  if (range && range.length >= 1) {
    start = Math.max(1, range[0])
  }
  if (range && range.length >= 2 && range[1] !== -1) {
    end = Math.min(lines.length, range[1])
  }
  const out: string[] = []
  for (let i = start; i <= end; i++) {
    out.push(`${i}\t${lines[i - 1] ?? ''}`)
  }
  return out.join('\n')
}

export async function grep(
  root: FileSystemDirectoryHandle,
  path: string,
  pattern: string,
  include: string,
): Promise<string> {
  const re = new RegExp(pattern)
  const includeRe = include ? globToRegExp(include) : null
  const dir = await getDir(root, path)
  const base = path.replace(/\/+$/, '').replace(/^\/+/, '')
  const matches: string[] = []
  for await (const { path: rel, handle } of walkFiles(dir, base)) {
    if (includeRe && !includeRe.test(rel) && !includeRe.test(basename(rel))) {
      continue
    }
    let text: string
    try {
      text = await (await handle.getFile()).text()
    } catch {
      continue
    }
    const fileLines = text.split('\n')
    for (let i = 0; i < fileLines.length; i++) {
      if (re.test(fileLines[i])) {
        matches.push(`${rel}:${i + 1}: ${fileLines[i]}`)
        if (matches.length >= kMaxGrepMatches) {
          matches.push(`... (truncated at ${kMaxGrepMatches} matches)`)
          return matches.join('\n')
        }
      }
    }
  }
  return matches.length ? matches.join('\n') : '(no matches)'
}

export async function glob(
  root: FileSystemDirectoryHandle,
  path: string,
  pattern: string,
): Promise<string> {
  const re = globToRegExp(pattern)
  const dir = await getDir(root, path)
  const base = path.replace(/\/+$/, '').replace(/^\/+/, '')
  const results: string[] = []
  for await (const { path: rel } of walkFiles(dir, base)) {
    if (re.test(rel) || re.test(basename(rel))) {
      results.push(rel)
      if (results.length >= kMaxGlobResults) {
        results.push(`... (truncated at ${kMaxGlobResults} results)`)
        break
      }
    }
  }
  return results.length ? results.join('\n') : '(no matches)'
}

export async function appendFile(
  root: FileSystemDirectoryHandle,
  path: string,
  content: string,
): Promise<string> {
  const existed = await fileExists(root, path)
  const previous = existed ? await readText(root, path) : ''
  await writeText(root, path, previous + content)
  return `Appended ${content.length} bytes to ${path}`
}

export async function createFile(
  root: FileSystemDirectoryHandle,
  path: string,
  fileText: string,
): Promise<string> {
  // Matches the text-editor `create` semantics: creates the file, overwriting
  // it if it already exists.
  const existed = await fileExists(root, path)
  await writeText(root, path, fileText)
  return `${existed ? 'Overwrote' : 'Created'} ${path} (${fileText.length} bytes)`
}

export async function strReplace(
  root: FileSystemDirectoryHandle,
  path: string,
  oldStr: string,
  newStr: string,
): Promise<string> {
  const text = await readText(root, path)
  const first = text.indexOf(oldStr)
  if (first === -1) {
    throw new Error('old_str was not found in the file')
  }
  if (text.includes(oldStr, first + 1)) {
    throw new Error('old_str is not unique in the file')
  }
  const updated =
    text.slice(0, first) + newStr + text.slice(first + oldStr.length)
  await writeText(root, path, updated)
  return `Replaced text in ${path}`
}

export async function insert(
  root: FileSystemDirectoryHandle,
  path: string,
  insertLine: number,
  newStr: string,
): Promise<string> {
  const text = await readText(root, path)
  const lines = text.split('\n')
  const at = Math.max(0, Math.min(insertLine, lines.length))
  const inserted = newStr.split('\n')
  lines.splice(at, 0, ...inserted)
  await writeText(root, path, lines.join('\n'))
  return `Inserted ${inserted.length} line(s) into ${path} after line ${at}`
}
