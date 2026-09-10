// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { spawnSync } from 'node:child_process'

import config from './config.ts'
import util from './util.js'
import * as Log from './log.ts'

export type PakDiffOptions = { maxDiffLines: number; prettier: boolean }
const maxDiffLineLength = 240
type TextFormatter = (
  content: string,
  options: { filepath: string; parser: string },
) => Promise<string>

export function isBinaryContent(content: Buffer): boolean {
  return content.includes(0) || content.toString('utf8').includes('\ufffd')
}

export function resourceFileName(resourceId: string): string {
  const dottedResourceId = resourceId.replaceAll('_', '.')
  const extension = path.extname(dottedResourceId).toLowerCase()
  return `${resourceId}${extension === '.bundle' ? '.js' : extension}`
}

export function classifyEntries(
  first: Map<string, Buffer>,
  second: Map<string, Buffer>,
) {
  const added: string[] = []
  const removed: string[] = []
  const changed: string[] = []
  for (const id of new Set([...first.keys(), ...second.keys()])) {
    if (!first.has(id)) added.push(id)
    else if (!second.has(id)) removed.push(id)
    else if (!first.get(id)!.equals(second.get(id)!)) changed.push(id)
  }
  return { added, removed, changed }
}

export async function normalizeText(
  content: Buffer,
  fileName: string,
  formatter?: TextFormatter,
): Promise<string> {
  if (!formatter) {
    const { default: prettier } = await import('prettier')
    const fileInfo = await prettier.getFileInfo(fileName, {
      withNodeModules: false,
    })
    if (fileInfo.ignored || !fileInfo.inferredParser) {
      return content.toString('utf8')
    }
    const options = await prettier.resolveConfig(fileName)
    return prettier.format(content.toString('utf8'), {
      ...options,
      filepath: fileName,
      parser: fileInfo.inferredParser,
    })
  }
  return formatter(content.toString('utf8'), {
    filepath: fileName,
    parser: path.extname(fileName).slice(1),
  })
}

export function truncateDiff(diff: string, maxLines: number): string {
  const lines = diff.split(/\r?\n/).map((line) => {
    if (line.length <= maxDiffLineLength) return line
    return `${line.slice(0, maxDiffLineLength)}... [line truncated]`
  })
  const truncatedLines = lines.length > maxLines
  const output = truncatedLines
    ? `${lines.slice(0, maxLines).join('\n')}\n... ${lines.length - maxLines} more lines`
    : lines.join('\n')
  return output
}

function runPakUtil(args: string[]): string {
  return util
    .run(
      'vpython3',
      [path.join(config.srcDir, 'tools/grit/pak_util.py'), ...args],
      util.mergeWithDefault({
        stdio: 'pipe',
      }),
    )
    .stdout.toString()
}

function extractedEntries(directory: string): Map<string, Buffer> {
  const entries = new Map<string, Buffer>()
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    if (entry.isFile()) {
      entries.set(entry.name, fs.readFileSync(path.join(directory, entry.name)))
    }
  }
  return entries
}

function extractPak(pak: string, buildDir: string): Map<string, Buffer> {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'pak-diff-'))
  const args = ['extract', pak, '-o', directory, '-t']
  if (fs.existsSync(`${pak}.info`)) {
    args.push('--brotli', path.join(buildDir, 'brotli'))
  } else {
    args.splice(args.indexOf('-t'), 1)
    Log.warn(`${pak}.info is missing; using numeric resource ids`)
  }
  runPakUtil(args)
  return extractedEntries(directory)
}

async function entryDiff(
  id: string,
  first: Buffer,
  second: Buffer,
  options: PakDiffOptions,
): Promise<string> {
  if (isBinaryContent(first) || isBinaryContent(second)) {
    return `${id}: binary differs)`
  }
  let firstText = first.toString('utf8')
  let secondText = second.toString('utf8')
  if (options.prettier) {
    const fileName = resourceFileName(id)
    try {
      firstText = await normalizeText(first, fileName)
    } catch {
      /* Keep raw text. */
    }
    try {
      secondText = await normalizeText(second, fileName)
    } catch {
      /* Keep raw text. */
    }
  }
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'pak-entry-diff-'))
  const firstFile = path.join(directory, 'first')
  const secondFile = path.join(directory, 'second')
  fs.writeFileSync(firstFile, firstText)
  fs.writeFileSync(secondFile, secondText)
  const result = spawnSync(
    'git',
    ['diff', '--no-index', '--', firstFile, secondFile],
    { encoding: 'utf8' },
  )
  fs.rmSync(directory, { recursive: true, force: true })
  return `${id}\n${truncateDiff(result.stdout || '', options.maxDiffLines)}`
}

export async function comparePakEntries(
  first: Map<string, Buffer>,
  second: Map<string, Buffer>,
  options: PakDiffOptions,
) {
  const classification = classifyEntries(first, second)
  const diffs = await Promise.all(
    classification.changed.map((id) =>
      entryDiff(id, first.get(id)!, second.get(id)!, options),
    ),
  )
  return { ...classification, diffs }
}

export async function comparePak(
  pak1: string,
  pak2: string,
  buildDir1: string,
  buildDir2: string,
  options: PakDiffOptions,
) {
  const first = extractPak(pak1, buildDir1)
  const second = extractPak(pak2, buildDir2)
  const result = await comparePakEntries(first, second, options)
  const firstPrint = runPakUtil(['print', pak1, '-t'])
  const secondPrint = runPakUtil(['print', pak2, '-t'])
  return { ...result, metadataChanged: firstPrint !== secondPrint }
}
