// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Registers the workspace file tools with Leo via the WebMCP API
// (document.modelContext). The editing tool follows Anthropic's text-editor
// tool ("str_replace_based_edit_tool"): a `command` enum (create / str_replace
// / insert) with matching parameter names, so it lands in the model's training
// distribution. Its `view` command is split out into a separate read-only
// `view` tool because the user grants permission per tool: if view and edit
// shared a tool, allowing the model to read files would also allow it to
// modify them. Search and repo-structure helpers with no text-editor analog
// are registered as separate auxiliary tools. All ops run against the
// FileSystemDirectoryHandle in file_ops, which is resolved lazily on each call
// (see registerTools).

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
  interface Document {
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

// |getRoot| is called each time a tool runs, rather than once up front, so the
// tools can be registered before the workspace has a folder and the folder can
// be chosen the first time one is actually needed.
export async function registerTools(
  getRoot: () => Promise<FileSystemDirectoryHandle>,
): Promise<void> {
  const mc = document.modelContext
  if (!mc) {
    console.error('[leo-workspace] document.modelContext is unavailable')
    return
  }

  const reg = (
    name: string,
    description: string,
    inputSchema: object,
    run: (
      root: FileSystemDirectoryHandle,
      input: Record<string, unknown>,
    ) => Promise<string>,
  ) =>
    mc.registerTool({
      name,
      description,
      inputSchema,
      execute: async (input) => {
        try {
          return await run(await getRoot(), input ?? {})
        } catch (e) {
          return `Error: ${e instanceof Error ? e.message : String(e)}`
        }
      },
    })

  // Read-only, and deliberately a separate tool from the editor below so that
  // permission to read the workspace doesn't also grant permission to write
  // it. Paths are relative to the workspace root and confined to it by the
  // File System Access API.
  await reg(
    'view',
    'Show a workspace file with 1-indexed line numbers, or list a directory. '
      + 'Optionally pass view_range=[start, end] (end -1 = end of file) to show '
      + 'part of a file. This tool only reads; use str_replace_based_edit_tool '
      + 'to change files.',
    schema(
      {
        path: str('File or directory path relative to the workspace root.'),
        view_range: intArray(
          'For a file: optional [start, end] 1-indexed line range. Use end of '
            + '-1 to read through the end of the file.',
        ),
      },
      ['path'],
    ),
    async (root, i) => {
      const path = asString(i.path)
      return (await ops.isDirectory(root, path))
        ? ops.listDir(root, path, 2)
        : ops.viewFile(
            root,
            path,
            Array.isArray(i.view_range)
              ? (i.view_range as number[])
              : undefined,
          )
    },
  )

  // The text-editor tool, dispatched on `command`. Every command writes.
  await reg(
    'str_replace_based_edit_tool',
    'Tool for creating and editing files in the workspace, modeled on '
      + "Anthropic's text editor tool. To read files or list directories, use "
      + 'the `view` tool instead. Commands:\n'
      + '- create: create a file with `file_text`, overwriting it if it exists. '
      + 'IMPORTANT: a large `file_text` gets truncated and rejected. For anything '
      + 'longer than a few lines, call create with an empty or very short '
      + '`file_text`, then add the rest with repeated append_file calls, one '
      + 'small chunk per call.\n'
      + '- str_replace: replace the unique occurrence of `old_str` with '
      + '`new_str`. `old_str` must match exactly once, including whitespace. '
      + 'Keep `new_str` small for the same reason.\n'
      + '- insert: insert `insert_text` after 1-indexed line `insert_line` (0 = '
      + 'start of file). Keep `insert_text` small.',
    schema(
      {
        command: strEnum(
          ['create', 'str_replace', 'insert'],
          'The edit command to run.',
        ),
        path: str('File path relative to the workspace root.'),
        file_text: str('For create: full contents of the file.'),
        old_str: str(
          'For str_replace: exact existing text to replace (must be unique).',
        ),
        new_str: str('For str_replace: replacement text.'),
        insert_line: int(
          'For insert: line number to insert after (0 = start of file).',
        ),
        insert_text: str('For insert: text to insert.'),
      },
      ['command', 'path'],
    ),
    async (root, i) => {
      const path = asString(i.path)
      switch (i.command) {
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
        case 'view':
          // Models trained on Anthropic's tool contract may still send this.
          // Never fall through to reading: this tool's permission grant is for
          // edits.
          throw new Error(
            'view is not a command of this tool; use the `view` tool instead',
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
    (root, i) =>
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
    (root, i) => ops.glob(root, asString(i.path), asString(i.pattern)),
  )

  await reg(
    'append_file',
    'Append `content` to the end of a workspace file, creating it if needed. '
      + 'Use this to build a file across several calls: create an empty file, '
      + 'then append_file one small chunk at a time. Prefer this over a single '
      + 'large create, whose big `file_text` argument gets truncated and '
      + 'rejected.',
    schema(
      {
        path: str('File path relative to the workspace root.'),
        content: str('Text to append to the end of the file.'),
      },
      ['path', 'content'],
    ),
    (root, i) => ops.appendFile(root, asString(i.path), asString(i.content)),
  )

  console.log('[leo-workspace] registered WebMCP tools')
}
