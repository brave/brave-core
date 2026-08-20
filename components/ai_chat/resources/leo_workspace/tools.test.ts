// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createFakeWorkspace, snapshotFakeWorkspace } from './test_file_system'
import { registerTools } from './tools'

// The tool shape registered with navigator.modelContext. Mirrors the internal
// ModelContextTool interface in tools.ts, which isn't exported.
interface RegisteredTool {
  name: string
  description: string
  inputSchema?: {
    type: string
    properties: Record<string, { type: string; enum?: string[] }>
    required: string[]
  }
  execute: (input: Record<string, unknown>) => Promise<unknown>
}

let registered: Map<string, RegisteredTool>
let registerTool: jest.Mock<Promise<void>, [RegisteredTool]>

function setModelContext(value: unknown) {
  Object.defineProperty(navigator, 'modelContext', {
    configurable: true,
    writable: true,
    value,
  })
}

/** Registers the tools against a fresh fake workspace and returns the root. */
async function setUpWorkspace(layout: Record<string, string> = {}) {
  const root = createFakeWorkspace(layout)
  await registerTools(root)
  return root
}

/** Runs a registered tool and returns its result as a string. */
async function run(name: string, input: Record<string, unknown> = {}) {
  const tool = registered.get(name)
  if (!tool) {
    throw new Error(`tool not registered: ${name}`)
  }
  return String(await tool.execute(input))
}

const kEditTool = 'str_replace_based_edit_tool'

beforeEach(() => {
  registered = new Map()
  registerTool = jest.fn(async (tool: RegisteredTool) => {
    registered.set(tool.name, tool)
  })
  setModelContext({ registerTool })
  // tools.ts logs on success and on a missing modelContext; keep the output
  // clean and let the tests assert on it.
  jest.spyOn(console, 'log').mockImplementation(() => {})
  jest.spyOn(console, 'error').mockImplementation(() => {})
})

afterEach(() => {
  delete navigator.modelContext
})

describe('registerTools', () => {
  it('registers the expected tools', async () => {
    await setUpWorkspace()
    expect([...registered.keys()]).toEqual([
      kEditTool,
      'grep',
      'glob',
      'append_file',
    ])
  })

  it('gives every tool a description and an object input schema', async () => {
    await setUpWorkspace()
    for (const tool of registered.values()) {
      expect(tool.description).toBeTruthy()
      expect(tool.inputSchema?.type).toBe('object')
    }
  })

  it('declares the text editor commands and required arguments', async () => {
    await setUpWorkspace()
    const schema = registered.get(kEditTool)!.inputSchema!
    // These names are Anthropic's text editor tool contract, so the model has
    // seen them in training. Changing them is not a refactor.
    expect(schema.properties.command.enum).toEqual([
      'view',
      'create',
      'str_replace',
      'insert',
    ])
    expect(schema.required).toEqual(['command', 'path'])
    expect(Object.keys(schema.properties)).toEqual([
      'command',
      'path',
      'file_text',
      'old_str',
      'new_str',
      'insert_line',
      'insert_text',
      'view_range',
    ])
  })

  it('requires only a pattern for the search tools', async () => {
    await setUpWorkspace()
    expect(registered.get('grep')!.inputSchema!.required).toEqual(['pattern'])
    expect(registered.get('glob')!.inputSchema!.required).toEqual(['pattern'])
    expect(registered.get('append_file')!.inputSchema!.required).toEqual([
      'path',
      'content',
    ])
  })

  it('does nothing but log when WebMCP is unavailable', async () => {
    setModelContext(undefined)
    await expect(registerTools(createFakeWorkspace())).resolves.toBeUndefined()
    expect(console.error).toHaveBeenCalledWith(
      expect.stringContaining('navigator.modelContext is unavailable'),
    )
    expect(registerTool).not.toHaveBeenCalled()
  })
})

describe('str_replace_based_edit_tool', () => {
  it('views a file with 1-indexed line numbers', async () => {
    await setUpWorkspace({ 'a.txt': 'one\ntwo' })
    expect(await run(kEditTool, { command: 'view', path: 'a.txt' })).toBe(
      '1\tone\n2\ttwo',
    )
  })

  it('passes view_range through to the file view', async () => {
    await setUpWorkspace({ 'a.txt': 'one\ntwo\nthree' })
    expect(
      await run(kEditTool, {
        command: 'view',
        path: 'a.txt',
        view_range: [2, -1],
      }),
    ).toBe('2\ttwo\n3\tthree')
  })

  it('ignores a view_range that is not an array', async () => {
    await setUpWorkspace({ 'a.txt': 'one\ntwo' })
    expect(
      await run(kEditTool, {
        command: 'view',
        path: 'a.txt',
        view_range: '2',
      }),
    ).toBe('1\tone\n2\ttwo')
  })

  it('lists a directory when the path is a directory', async () => {
    await setUpWorkspace({ 'src/a.ts': '', 'src/b.ts': '' })
    expect(await run(kEditTool, { command: 'view', path: 'src' })).toBe(
      'src/a.ts\nsrc/b.ts',
    )
  })

  it('lists the workspace root for an empty path', async () => {
    await setUpWorkspace({ 'a.txt': '' })
    expect(await run(kEditTool, { command: 'view', path: '' })).toBe('a.txt')
  })

  it('creates a file', async () => {
    const root = await setUpWorkspace()
    expect(
      await run(kEditTool, {
        command: 'create',
        path: 'src/a.ts',
        file_text: 'hello',
      }),
    ).toBe('Created src/a.ts (5 bytes)')
    expect(snapshotFakeWorkspace(root)).toEqual({ 'src/a.ts': 'hello' })
  })

  it('creates an empty file when file_text is missing', async () => {
    const root = await setUpWorkspace()
    expect(await run(kEditTool, { command: 'create', path: 'a.txt' })).toBe(
      'Created a.txt (0 bytes)',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': '' })
  })

  it('replaces text', async () => {
    const root = await setUpWorkspace({ 'a.txt': 'one\ntwo' })
    expect(
      await run(kEditTool, {
        command: 'str_replace',
        path: 'a.txt',
        old_str: 'two',
        new_str: 'three',
      }),
    ).toBe('Replaced text in a.txt')
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': 'one\nthree' })
  })

  it('inserts text after the given line', async () => {
    const root = await setUpWorkspace({ 'a.txt': 'one\nthree' })
    expect(
      await run(kEditTool, {
        command: 'insert',
        path: 'a.txt',
        insert_line: 1,
        insert_text: 'two',
      }),
    ).toBe('Inserted 1 line(s) into a.txt after line 1')
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': 'one\ntwo\nthree' })
  })

  it('inserts at the start when insert_line is missing', async () => {
    const root = await setUpWorkspace({ 'a.txt': 'two' })
    expect(
      await run(kEditTool, {
        command: 'insert',
        path: 'a.txt',
        insert_text: 'one',
      }),
    ).toBe('Inserted 1 line(s) into a.txt after line 0')
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': 'one\ntwo' })
  })

  it('inserts at the start when insert_line is not a number', async () => {
    // The model sometimes sends numbers as strings; those fall back to 0
    // rather than throwing.
    const root = await setUpWorkspace({ 'a.txt': 'two' })
    expect(
      await run(kEditTool, {
        command: 'insert',
        path: 'a.txt',
        insert_line: '1',
        insert_text: 'one',
      }),
    ).toBe('Inserted 1 line(s) into a.txt after line 0')
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': 'one\ntwo' })
  })
})

describe('grep tool', () => {
  it('searches the whole workspace by default', async () => {
    await setUpWorkspace({ 'a.txt': 'needle', 'sub/b.txt': 'needle' })
    const result = await run('grep', { pattern: 'needle' })
    expect(result.split('\n').sort()).toEqual([
      'a.txt:1: needle',
      'sub/b.txt:1: needle',
    ])
  })

  it('honours path and include', async () => {
    await setUpWorkspace({
      'a.txt': 'needle',
      'sub/b.txt': 'needle',
      'sub/c.ts': 'needle',
    })
    expect(await run('grep', { pattern: 'needle', path: 'sub' })).toContain(
      'sub/b.txt:1: needle',
    )
    expect(await run('grep', { pattern: 'needle', include: '*.ts' })).toBe(
      'sub/c.ts:1: needle',
    )
  })

  it('reports no matches', async () => {
    await setUpWorkspace({ 'a.txt': 'nothing' })
    expect(await run('grep', { pattern: 'needle' })).toBe('(no matches)')
  })
})

describe('glob tool', () => {
  it('matches paths under the workspace', async () => {
    await setUpWorkspace({ 'a.txt': '', 'sub/b.ts': '' })
    expect(await run('glob', { pattern: '**/*.ts' })).toBe('sub/b.ts')
  })

  it('honours path', async () => {
    await setUpWorkspace({ 'a.ts': '', 'sub/b.ts': '' })
    expect(await run('glob', { pattern: '*.ts', path: 'sub' })).toBe('sub/b.ts')
  })
})

describe('append_file tool', () => {
  it('appends to a file, creating it if needed', async () => {
    const root = await setUpWorkspace()
    expect(await run('append_file', { path: 'a.txt', content: 'one\n' })).toBe(
      'Appended 4 bytes to a.txt',
    )
    expect(await run('append_file', { path: 'a.txt', content: 'two\n' })).toBe(
      'Appended 4 bytes to a.txt',
    )
    expect(snapshotFakeWorkspace(root)).toEqual({ 'a.txt': 'one\ntwo\n' })
  })
})

// A tool that throws would surface to the model as a failed tool call with no
// explanation, so every tool turns failures into a readable result instead.
describe('error handling', () => {
  it('returns the error message instead of rejecting', async () => {
    await setUpWorkspace({ 'a.txt': 'one' })
    expect(
      await run(kEditTool, {
        command: 'str_replace',
        path: 'a.txt',
        old_str: 'missing',
        new_str: 'x',
      }),
    ).toBe('Error: old_str was not found in the file')
  })

  it('reports an unknown command', async () => {
    await setUpWorkspace()
    expect(await run(kEditTool, { command: 'delete', path: 'a.txt' })).toBe(
      'Error: unknown command: delete',
    )
    expect(await run(kEditTool, { path: 'a.txt' })).toBe(
      'Error: unknown command: undefined',
    )
  })

  it('reports a path that escapes the workspace', async () => {
    await setUpWorkspace({ 'a.txt': 'one' })
    expect(await run(kEditTool, { command: 'view', path: '../a.txt' })).toMatch(
      /^Error: path escapes the workspace root/,
    )
    expect(
      await run('append_file', { path: '../a.txt', content: 'x' }),
    ).toMatch(/^Error: path escapes the workspace root/)
  })

  it('reports a missing file', async () => {
    await setUpWorkspace()
    expect(await run(kEditTool, { command: 'view', path: 'nope.txt' })).toMatch(
      /^Error: /,
    )
  })

  it('reports an invalid regular expression', async () => {
    await setUpWorkspace({ 'a.txt': 'one' })
    expect(await run('grep', { pattern: '([' })).toMatch(/^Error: /)
  })

  it('tolerates being called with no arguments at all', async () => {
    await setUpWorkspace()
    const tool = registered.get(kEditTool)!
    await expect(
      tool.execute(undefined as unknown as Record<string, unknown>),
    ).resolves.toBe('Error: unknown command: undefined')
  })
})
