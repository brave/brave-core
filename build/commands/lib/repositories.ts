// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import path from 'node:path'
import config from './config.ts'

// The gn source-root prefix every entry in `.repositories.cfg` carries, `//`
// being Chromium's `src/`, exactly as a gn label spells it.
const sourceRootPrefix = '//'

// The name of the file, which lives at the root of `patches/`.
const repositoriesFileName = '.repositories.cfg'

/** A type representing a repository listed in `.repositories.cfg`. */
export interface PatchedRepository {
  /** The entry as the file spells it, e.g. `//v8`, or `//` for Chromium. */
  label: string

  /**
   * Absolute path of the repository: Chromium's `src` itself, or whichever
   * directory under it the entry names.
   */
  path: string

  /** Absolute path of the directory holding this repository's patches. */
  patchDir: string

  /** Whether this is Chromium's own `src`. */
  isChromium: boolean
}

/**
 * The repository root relative to `src/`.
 */
function relativePathOf(repository: PatchedRepository): string {
  return repository.label.slice(sourceRootPrefix.length)
}

/**
 * The file listing every repository brave-core patches.
 */
export function getRepositoriesFilePath(): string {
  return path.join(config.braveCoreDir, 'patches', repositoriesFileName)
}

/**
 * Parses the repositories file into repository paths relative to `src/`.
 *
 * This is similar to other `.cfg` parsing routines, with comments being
 * supported.
 *
 * @param contents The file's contents.
 * @param filePath The file's path, named in errors.
 * @throws If any entry is malformed, duplicated, or if `//` is missing.
 */
export function parsePatchedRepositories(
  contents: string,
  filePath: string,
): string[] {
  const relativePaths: string[] = []
  for (const [index, rawLine] of contents.split('\n').entries()) {
    const lineNumber = index + 1
    const line = rawLine.replace(/#.*$/, '').trim() // Removing comments.
    if (line.length === 0) {
      continue
    }
    if (!line.startsWith(sourceRootPrefix)) {
      throw new Error(
        `${filePath}:${lineNumber}: repository path '${line}' must be `
          + `source-absolute, i.e. start with '${sourceRootPrefix}', which is `
          + `'src/'`,
      )
    }
    const relativePath = line
      .slice(sourceRootPrefix.length)
      .replace(/^\/+|\/+$/g, '')
    // A path is a prefix of both a `rewrite/` and a `patches/` path, so
    // anything that does not stay inside those trees cannot be resolved
    // against them.
    if (relativePath.split('/').includes('..')) {
      throw new Error(
        `${filePath}:${lineNumber}: repository path '${line}' must stay under `
          + `'${sourceRootPrefix}' and cannot traverse upwards`,
      )
    }
    if (relativePaths.includes(relativePath)) {
      throw new Error(
        `${filePath}:${lineNumber}: repository path '${line}' is listed more `
          + `than once`,
      )
    }
    relativePaths.push(relativePath)
  }

  if (!relativePaths.includes('')) {
    throw new Error(
      `${filePath}: does not list Chromium's own 'src', which every patch `
        + `outside another repository belongs to. Add a '${sourceRootPrefix}' `
        + `line for it`,
    )
  }

  return relativePaths
}

/**
 * Every repository brave-core patches, in the order `.repositories.cfg` lists
 * them.
 *
 * @throws If the file is missing or any entry is malformed.
 */
export function getPatchedRepositories(): PatchedRepository[] {
  const filePath = getRepositoriesFilePath()
  const patchesDir = path.dirname(filePath)
  const contents = fs.readFileSync(filePath, 'utf-8')
  return parsePatchedRepositories(contents, filePath).map((relativePath) => {
    // Chromium's own `src` is the empty path, which contributes no segments
    // to either directory.
    const segments = relativePath.split('/').filter(Boolean)
    return {
      label: `${sourceRootPrefix}${relativePath}`,
      path: path.join(config.srcDir, ...segments),
      patchDir: path.join(patchesDir, ...segments),
      isChromium: relativePath.length === 0,
    }
  })
}

/**
 * Splits a `src/`-relative source path into the repository holding it and the
 * path of the source within that repository.
 *
 * The most specific repository wins, so one nested inside another claims its
 * own sources rather than being swallowed by the one above it, and anything
 * matching no repository falls to Chromium's `src`.
 *
 * @param repositories As returned by `getPatchedRepositories`.
 * @param sourcePath The source path relative to `src/`, posix-separated.
 * @throws If `repositories` holds no entry for Chromium's `src`.
 */
export function splitSourcePath(
  repositories: PatchedRepository[],
  sourcePath: string,
): { repository: PatchedRepository; relativePath: string } {
  let match: PatchedRepository | undefined
  let matchPrefix = ''
  for (const repository of repositories) {
    if (repository.isChromium) {
      continue
    }
    const prefix = relativePathOf(repository)
    if (!sourcePath.startsWith(`${prefix}/`)) {
      continue
    }
    if (match === undefined || prefix.length > matchPrefix.length) {
      match = repository
      matchPrefix = prefix
    }
  }
  if (match !== undefined) {
    return {
      repository: match,
      relativePath: sourcePath.slice(matchPrefix.length + 1),
    }
  }

  const chromium = repositories.find((repository) => repository.isChromium)
  if (chromium === undefined) {
    throw new Error(
      `No repository for '${sourcePath}': the repositories list does not `
        + `include Chromium's own 'src'`,
    )
  }
  return { repository: chromium, relativePath: sourcePath }
}

/**
 * Qualifies a path within `repository` back into a `src/`-relative source
 * path, the inverse of `splitSourcePath`.
 *
 * This is what names a source the way the whole checkout sees it, for a log
 * line or a message that has to be meaningful outside the repository it came
 * from.
 *
 * @param repository The repository `relativePath` is within.
 * @param relativePath A path relative to that repository, posix-separated.
 */
export function joinSourcePath(
  repository: PatchedRepository,
  relativePath: string,
): string {
  if (repository.isChromium) {
    return relativePath
  }
  return `${relativePathOf(repository)}/${relativePath}`
}
