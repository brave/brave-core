// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Registers the workspace file tools with Leo via the WebMCP API
// (navigator.modelContext). The primary editing tool follows Anthropic's
// text-editor tool ("str_replace_based_edit_tool"): a single tool with a
// `command` enum (view / create / str_replace / insert) and matching parameter
// names, so it lands in the model's training distribution. Search and
// repo-structure helpers with no text-editor analog are registered as separate
// auxiliary tools. All ops run against the FileSystemDirectoryHandle in
// file_ops.

import * as ops from './file_ops'

interface ModelContextTool {
  name: string
  description: string
  inputSchema?: object
  execute: (input: Record<string, unknown>) => Promise<unknown>
}
interface ModelContext {
  registerTool(tool: ModelContextTool): Promise<void>
}
declare global {
  interface Navigator {
    modelContext?: ModelContext
  }
}

function schema(
  properties: Record<string, object>,
  required?: string[],
): object {
  return { type: 'object', properties, required: required ?? [] }
}
const str = (description: string) => ({ type: 'string', description })
const int = (description: string) => ({ type: 'integer', description })
const strEnum = (values: string[], description: string) => ({
  type: 'string',
  enum: values,
  description,
})
const intArray = (description: string) => ({
  type: 'array',
  description,
  items: { type: 'integer' },
})

function asString(v: unknown): string {
  return typeof v === 'string' ? v : ''
}
function asInt(v: unknown, fallback: number): number {
  return typeof v === 'number' ? v : fallback
}

export async function registerTools(
  root: FileSystemDirectoryHandle,
): Promise<void> {
  const mc = navigator.modelContext
  if (!mc) {
    console.error('[workspace] navigator.modelContext is unavailable')
    return
  }

  const reg = (
    name: string,
    description: string,
    inputSchema: object,
    run: (input: Record<string, unknown>) => Promise<string>,
  ) =>
    mc.registerTool({
      name,
      description,
      inputSchema,
      execute: async (input) => {
        try {
          return await run(input ?? {})
        } catch (e) {
          return `Error: ${e instanceof Error ? e.message : String(e)}`
        }
      },
    })

  // The text-editor tool: one tool, dispatched on `command`. Paths are relative
  // to the workspace root and confined to it by the File System Access API.
  await reg(
    'str_replace_based_edit_tool',
    'Tool for viewing, creating and editing files in the workspace, modeled '
      + "on Anthropic's text editor tool. Commands:\n"
      + '- view: show a file with 1-indexed line numbers, or list a directory. '
      + 'Optionally pass view_range=[start, end] (end -1 = end of file) to show '
      + 'part of a file.\n'
      + '- create: create a file with `file_text`, overwriting it if it exists. '
      + 'Keep `file_text` modest; a very large single-shot body can exceed the '
      + 'model output limit and be rejected, so build large files incrementally '
      + '(create a short file, then extend it with insert).\n'
      + '- str_replace: replace the unique occurrence of `old_str` with '
      + '`new_str`. `old_str` must match exactly once, including whitespace.\n'
      + '- insert: insert `insert_text` after 1-indexed line `insert_line` (0 = '
      + 'start of file).',
    schema(
      {
        command: strEnum(
          ['view', 'create', 'str_replace', 'insert'],
          'The edit command to run.',
        ),
        path: str('File or directory path relative to the workspace root.'),
        file_text: str('For create: full contents of the file.'),
        old_str: str(
          'For str_replace: exact existing text to replace (must be unique).',
        ),
        new_str: str('For str_replace: replacement text.'),
        insert_line: int(
          'For insert: line number to insert after (0 = start of file).',
        ),
        insert_text: str('For insert: text to insert.'),
        view_range: intArray(
          'For view on a file: optional [start, end] 1-indexed line range. '
            + 'Use end of -1 to read through the end of the file.',
        ),
      },
      ['command', 'path'],
    ),
    async (i) => {
      const path = asString(i.path)
      switch (i.command) {
        case 'view':
          return (await ops.isDirectory(root, path))
            ? ops.listDir(root, path, 2)
            : ops.viewFile(
                root,
                path,
                Array.isArray(i.view_range)
                  ? (i.view_range as number[])
                  : undefined,
              )
        case 'create':
          return ops.createFile(root, path, asString(i.file_text))
        case 'str_replace':
          return ops.strReplace(
            root,
            path,
            asString(i.old_str),
            asString(i.new_str),
          )
        case 'insert':
          return ops.insert(
            root,
            path,
            asInt(i.insert_line, 0),
            asString(i.insert_text),
          )
        default:
          throw new Error(`unknown command: ${String(i.command)}`)
      }
    },
  )

  // Auxiliary tools with no text-editor analog.
  await reg(
    'grep',
    'Search file contents within the workspace for a regular expression.',
    schema(
      {
        pattern: str('Regular expression to search for.'),
        path: str(
          'Directory to search under, relative to the workspace root. Empty '
            + 'searches the whole workspace.',
        ),
        include: str(
          'Optional glob restricting which files are searched by their '
            + 'relative path, e.g. *.ts',
        ),
      },
      ['pattern'],
    ),
    (i) =>
      ops.grep(
        root,
        asString(i.path),
        asString(i.pattern),
        asString(i.include),
      ),
  )

  await reg(
    'glob',
    'Find files in the workspace whose relative path matches a glob.',
    schema(
      {
        pattern: str('Glob to match file paths, e.g. **/*.ts'),
        path: str(
          'Directory to search under, relative to the workspace root. Empty '
            + 'searches the whole workspace.',
        ),
      },
      ['pattern'],
    ),
    (i) => ops.glob(root, asString(i.path), asString(i.pattern)),
  )

  console.log('[workspace] registered WebMCP tools')
}
