// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import crypto from 'node:crypto'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import { spawnSync } from 'node:child_process'
import config from './config.ts'
import { snapshotConfig } from './configSnapshot.ts'
import { isCI } from './ciDetect.ts'
import * as Log from './log.ts'
import util, { prefixPatchPaths } from './util.ts'

jest.mock('./log.ts', () => ({
  command: jest.fn(),
  status: jest.fn(),
  warn: jest.fn(),
  error: jest.fn(),
  progressStart: jest.fn(),
  progressFinish: jest.fn(),
  progressScope: jest.fn(),
  progressScopeAsync: jest.fn(),
}))

// On Windows commands run through `cmd /c`, which does not quote a path with
// spaces such as `C:\Program Files\nodejs\node.exe`, so resolve it via PATH.
const node = process.platform === 'win32' ? 'node' : process.execPath

type PatchStatus = { path?: string; patchPath: string }

describe('prefixPatchPaths', function () {
  test('leaves a status carrying no path alone', function () {
    // A patch too malformed to read the files it applies to reports no path,
    // which used to be joined all the same, failing the whole apply with
    // `ERR_INVALID_ARG_TYPE` instead of reporting the patch that broke.
    const status: PatchStatus = { patchPath: '/patches/broken.patch' }

    expect(() =>
      prefixPatchPaths([status], 'third_party', 'devtools-frontend', 'src'),
    ).not.toThrow()
    expect(status.path).toBeUndefined()
  })

  test('prefixes the repo onto a status carrying a path', function () {
    const status: PatchStatus = {
      patchPath: '/patches/scripts-build-ts_library.py.patch',
      path: path.join('scripts', 'build', 'ts_library.py'),
    }

    prefixPatchPaths([status], 'third_party', 'devtools-frontend', 'src')

    expect(status.path).toBe(
      path.join(
        'third_party',
        'devtools-frontend',
        'src',
        'scripts',
        'build',
        'ts_library.py',
      ),
    )
  })

  test('prefixes only the statuses carrying a path', function () {
    const withPath: PatchStatus = {
      patchPath: '/patches/file1.patch',
      path: 'file1',
    }
    const withoutPath: PatchStatus = { patchPath: '/patches/broken.patch' }

    prefixPatchPaths([withPath, withoutPath], 'v8')

    expect(withPath.path).toBe(path.join('v8', 'file1'))
    expect(withoutPath.path).toBeUndefined()
  })

  test('handles a repo with no patches at all', function () {
    expect(() => prefixPatchPaths([], 'third_party', 'ffmpeg')).not.toThrow()
  })
})

function git(cwd: string, ...args: string[]): string {
  const result = spawnSync(
    'git',
    [
      '-c',
      'user.name=Test',
      '-c',
      'user.email=test@example.com',
      '-c',
      'commit.gpgsign=false',
      ...args,
    ],
    { cwd, encoding: 'utf8', env: process.env },
  )
  if (result.status !== 0) {
    throw new Error(`git ${args.join(' ')} failed:\n${result.stderr}`)
  }
  return result.stdout.trim()
}

function withPlatform<T>(platform: string, callback: () => T): T {
  const original = Object.getOwnPropertyDescriptor(process, 'platform')!
  Object.defineProperty(process, 'platform', { value: platform })
  try {
    return callback()
  } finally {
    Object.defineProperty(process, 'platform', original)
  }
}

describe('util', () => {
  let tmpDir: string
  let restoreConfig: () => void
  let savedEnv: Record<string, string | undefined>
  let exit: jest.SpiedFunction<typeof process.exit>
  let consoleLog: jest.SpiedFunction<typeof console.log>
  let consoleError: jest.SpiedFunction<typeof console.error>

  const write = (file: string, content = '') => {
    fs.mkdirSync(path.dirname(file), { recursive: true })
    fs.writeFileSync(file, content)
    return file
  }

  // Makes a repo with one commit on `main`.
  const initRepo = (dir: string) => {
    fs.mkdirSync(dir, { recursive: true })
    git(dir, 'init', '-q', '-b', 'main')
    write(path.join(dir, 'first.txt'), 'first')
    git(dir, 'add', '.')
    git(dir, 'commit', '-q', '-m', 'first')
  }

  beforeEach(() => {
    restoreConfig = snapshotConfig()
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'brave-util-'))
    config.srcDir = path.join(tmpDir, 'src')
    config.rootDir = tmpDir

    // Ignore the user's git config.
    savedEnv = {
      GIT_CONFIG_GLOBAL: process.env.GIT_CONFIG_GLOBAL,
      GIT_CONFIG_NOSYSTEM: process.env.GIT_CONFIG_NOSYSTEM,
    }
    process.env.GIT_CONFIG_GLOBAL = write(path.join(tmpDir, 'gitconfig'))
    process.env.GIT_CONFIG_NOSYSTEM = '1'

    // `util.run` exits the process on failure; surface that as a test failure
    // instead of killing the worker.
    exit = jest.spyOn(process, 'exit').mockImplementation(((code: number) => {
      throw new Error(`process.exit(${code}) called`)
    }) as never)
    consoleLog = jest.spyOn(console, 'log').mockImplementation(() => {})
    consoleError = jest.spyOn(console, 'error').mockImplementation(() => {})
    jest
      .mocked(Log.progressScopeAsync)
      .mockImplementation(async (_message, callable) => callable())
  })

  afterEach(() => {
    jest.restoreAllMocks()
    restoreConfig()
    for (const [key, value] of Object.entries(savedEnv)) {
      if (value === undefined) {
        delete process.env[key]
      } else {
        process.env[key] = value
      }
    }
    fs.rmSync(tmpDir, { recursive: true, force: true })
  })

  describe('walkSync', () => {
    it('lists files recursively, optionally filtered', () => {
      write(path.join(tmpDir, 'w', 'a.txt'))
      write(path.join(tmpDir, 'w', 'sub', 'b.cc'))
      write(path.join(tmpDir, 'w', 'sub', 'deeper', 'c.txt'))

      const dir = path.join(tmpDir, 'w')
      expect(util.walkSync(dir).sort()).toEqual(
        [
          path.join(dir, 'a.txt'),
          path.join(dir, 'sub', 'b.cc'),
          path.join(dir, 'sub', 'deeper', 'c.txt'),
        ].sort(),
      )
      expect(
        util.walkSync(dir, (file) => file.endsWith('.txt')).sort(),
      ).toEqual(
        [
          path.join(dir, 'a.txt'),
          path.join(dir, 'sub', 'deeper', 'c.txt'),
        ].sort(),
      )
    })
  })

  describe('platform helpers', () => {
    it('appends .exe only on win32', () => {
      expect(withPlatform('linux', () => util.appendExeIfWin32('gn'))).toBe(
        'gn',
      )
      expect(withPlatform('win32', () => util.appendExeIfWin32('gn'))).toBe(
        'gn.exe',
      )
    })

    it('runs commands through cmd on win32', () => {
      // A real Windows host does have `cmd`, so the spawn would succeed.
      if (process.platform === 'win32') {
        return
      }
      // There is no `cmd` here, so the spawn fails, and the error says what
      // was attempted.
      const prog = withPlatform('win32', () =>
        util.runProcess('echo', ['hi'], {}, true),
      )
      expect(prog.error).toMatchObject({
        path: 'cmd',
        spawnargs: ['/c', 'echo', 'hi'],
      })
    })
  })

  describe('mergeWithDefault', () => {
    it('layers the options over the defaults', () => {
      const merged = util.mergeWithDefault({ cwd: '/somewhere' })
      expect(merged.cwd).toBe('/somewhere')
      expect(merged.env).toEqual(config.defaultOptions.env)
    })
  })

  describe('JSON files', () => {
    it('round-trips', () => {
      const file = path.join(tmpDir, 'value.json')
      util.writeJSON(file, { a: [1, 2] })
      expect(fs.readFileSync(file, 'utf8')).toBe(
        `${JSON.stringify({ a: [1, 2] }, null, 2)}\n`,
      )
      expect(util.readJSON(file)).toEqual({ a: [1, 2] })
    })

    it('falls back to the default for a missing or broken file', () => {
      expect(util.readJSON(path.join(tmpDir, 'missing.json'), 'x')).toBe('x')
      expect(util.readJSON(path.join(tmpDir, 'missing.json'))).toBeUndefined()
      const broken = write(path.join(tmpDir, 'broken.json'), '{nope')
      expect(util.readJSON(broken, { fallback: true })).toEqual({
        fallback: true,
      })
    })
  })

  describe('writeFileIfModified', () => {
    it('writes only when the content changes', () => {
      const file = path.join(tmpDir, 'out.txt')
      expect(util.writeFileIfModified(file, 'one')).toBe(true)
      expect(fs.readFileSync(file, 'utf8')).toBe('one')
      expect(util.writeFileIfModified(file, 'one')).toBe(false)
      expect(util.writeFileIfModified(file, 'two')).toBe(true)
      expect(fs.readFileSync(file, 'utf8')).toBe('two')
      expect(fs.existsSync(`${file}.tmp`)).toBe(false)
    })
  })

  describe('calculateFileChecksum', () => {
    it.each([0, 5, 8191, 8192, 8193, 20000])(
      'matches md5 for %i bytes',
      (size) => {
        const content = crypto.randomBytes(size)
        const file = path.join(tmpDir, 'data.bin')
        fs.writeFileSync(file, content)
        expect(util.calculateFileChecksum(file)).toBe(
          crypto.createHash('md5').update(content).digest('hex'),
        )
      },
    )
  })

  describe('readLines', () => {
    const collect = async (file: string, maxLines?: number) => {
      const lines: string[] = []
      for await (const line of util.readLines(file, maxLines)) {
        lines.push(line)
      }
      return lines
    }

    it('reads every line, handling CRLF', async () => {
      const file = write(path.join(tmpDir, 'lines.txt'), 'a\r\nb\nc\n')
      expect(await collect(file)).toEqual(['a', 'b', 'c'])
    })

    it('stops at the line limit', async () => {
      const file = write(path.join(tmpDir, 'lines.txt'), '1\n2\n3\n4\n5\n')
      expect(await collect(file, 3)).toEqual(['1', '2', '3'])
      expect(await collect(file, 10)).toEqual(['1', '2', '3', '4', '5'])
    })
  })

  describe('generateInstrumentationFile', () => {
    it('lists the source files under the cwd', async () => {
      const cwd = process.cwd()
      write(path.join(tmpDir, 'inst', 'a.cc'))
      write(path.join(tmpDir, 'inst', 'sub', 'b.h'))
      write(path.join(tmpDir, 'inst', 'sub', 'c.mm'))
      write(path.join(tmpDir, 'inst', 'ignored.txt'))
      process.chdir(path.join(tmpDir, 'inst'))
      try {
        const output = path.join(tmpDir, 'nested', 'files-to-instrument.txt')
        await util.generateInstrumentationFile(output)
        expect(fs.readFileSync(output, 'utf8').split('\n').sort()).toEqual(
          ['a.cc', path.join('sub', 'b.h'), path.join('sub', 'c.mm')]
            .map((file) => `../../brave/${file}`)
            .sort(),
        )
      } finally {
        process.chdir(cwd)
      }
    })
  })

  describe('running processes', () => {
    // Scripts go through a file rather than `-e`: on Windows `cmd /c` does not
    // quote arguments, so code containing spaces would be split.
    let scriptCount = 0
    function script(code: string): string {
      const file = path.join(tmpDir, `script${scriptCount++}.js`)
      fs.writeFileSync(file, code)
      return file
    }

    it('runProcess logs the command unless told not to', () => {
      const args = [script('0')]
      const prog = util.runProcess(node, args, { cwd: tmpDir })
      expect(prog.status).toBe(0)
      expect(Log.command).toHaveBeenCalledWith(tmpDir, node, args)

      jest.mocked(Log.command).mockClear()
      util.runProcess(node, args, {}, true)
      expect(Log.command).not.toHaveBeenCalled()
    })

    it('run returns the result of a successful command', () => {
      const prog = util.run(node, [script('console.log("hi")')], {
        encoding: 'utf8',
      })
      expect(prog.status).toBe(0)
      expect(prog.stdout).toBe('hi\n')
    })

    it('run exits on failure, dumping the output', () => {
      expect(() =>
        util.run(node, [
          script('console.log("out"); console.error("err"); process.exit(3)'),
        ]),
      ).toThrow('process.exit(1) called')
      expect(exit).toHaveBeenCalledWith(1)
      expect(consoleLog).toHaveBeenCalledWith('out\n')
      expect(consoleError).toHaveBeenCalledWith('err\n')
    })

    it('run also dumps the command when it was not logged', () => {
      const args = [script('process.exit(2)')]
      expect(() => util.run(node, args, { skipLogging: true })).toThrow(
        'process.exit(1) called',
      )
      expect(consoleLog).toHaveBeenCalledWith(
        node,
        args,
        'exited with status',
        2,
        expect.anything(),
      )
    })

    it('run hands a failure back with continueOnFail', () => {
      const prog = util.run(node, [script('process.exit(4)')], {
        continueOnFail: true,
      })
      expect(prog.status).toBe(4)
      expect(exit).not.toHaveBeenCalled()
    })

    it('runGit returns trimmed output, or nothing on failure', () => {
      initRepo(path.join(tmpDir, 'repo'))
      const repo = path.join(tmpDir, 'repo')
      expect(util.runGit(repo, ['rev-parse', '--abbrev-ref', 'HEAD'])).toBe(
        'main',
      )
      expect(util.runGit(repo, ['rev-parse', '--verify', 'nope'], true)).toBe(
        '',
      )
      expect(() =>
        util.runGit(repo, ['rev-parse', '--verify', 'nope']),
      ).toThrow('process.exit(1) called')
    })

    it('getGitReadableLocalRef describes HEAD', () => {
      initRepo(path.join(tmpDir, 'repo'))
      expect(util.getGitReadableLocalRef(path.join(tmpDir, 'repo'))).toMatch(
        /^[0-9a-f]+ \(HEAD -> main\)$/,
      )
    })

    it('runAsync resolves with stdout and reports the command', async () => {
      const args = [script('console.log("hi")')]
      const stdout = await util.runAsync(node, args, { cwd: tmpDir })
      expect(stdout).toBe('hi\n')
      expect(Log.command).toHaveBeenCalledWith(tmpDir, node, args)
    })

    it('runAsync can stay quiet, or print the output when verbose', async () => {
      const args = [script('console.log("o"); console.error("e")')]
      await util.runAsync(node, args, { verbose: false })
      expect(Log.command).not.toHaveBeenCalled()
      expect(consoleLog).not.toHaveBeenCalled()

      await util.runAsync(node, args, { verbose: true })
      expect(consoleLog).toHaveBeenCalledWith('o\n')
      expect(consoleError).toHaveBeenCalledWith('e\n')
    })

    it('runAsync streams lines and lets the caller see the process', async () => {
      const out: string[] = []
      const err: string[] = []
      const onSpawn = jest.fn()
      const stdout = await util.runAsync(
        node,
        [script('console.log("a\\nb"); console.error("c")')],
        {
          onStdOutLine: (line) => out.push(line),
          onStdErrLine: (line) => err.push(line),
          onSpawn,
        },
      )
      expect(out).toEqual(['a', 'b'])
      expect(err).toEqual(['c'])
      // Lines are handed out instead of being collected.
      expect(stdout).toBe('')
      expect(onSpawn).toHaveBeenCalledTimes(1)
    })

    it('runAsync rejects with the output of a failed process', async () => {
      exit.mockImplementation((() => undefined) as never)
      const failure = util.runAsync(node, [
        script('console.log("o"); console.error("e"); process.exit(3)'),
      ])
      await expect(failure).rejects.toMatchObject({
        message: `Program ${node} exited with error code 3.`,
        stdout: 'o\n',
        stderr: 'e\n',
        statusCode: 3,
      })
      expect(exit).toHaveBeenCalledWith(3)
    })

    it('runAsync leaves the exit to the caller with continueOnFail', async () => {
      await expect(
        util.runAsync(node, [script('process.exit(5)')], {
          continueOnFail: true,
        }),
      ).rejects.toMatchObject({ statusCode: 5 })
      expect(exit).not.toHaveBeenCalled()
    })

    it('runAsync exits like a signalled child did', async () => {
      // Relies on POSIX signal semantics.
      if (process.platform === 'win32') {
        return
      }
      exit.mockImplementation((() => undefined) as never)
      await util.runAsync(node, [
        script(
          'process.kill(process.pid, "SIGTERM"); setTimeout(() => {}, 10000)',
        ),
      ])
      expect(exit).toHaveBeenCalledWith(128 + os.constants.signals.SIGTERM)
    })

    it('runGitAsync logs the error only when asked to', async () => {
      initRepo(path.join(tmpDir, 'repo'))
      const repo = path.join(tmpDir, 'repo')
      await expect(
        util.runGitAsync(repo, ['rev-parse', '--verify', 'nope']),
      ).rejects.toMatchObject({ statusCode: expect.any(Number) })
      expect(consoleError).not.toHaveBeenCalled()

      await expect(
        util.runGitAsync(repo, ['rev-parse', '--verify', 'nope'], false, true),
      ).rejects.toThrow()
      expect(consoleError).toHaveBeenCalledWith(
        'Git arguments were: rev-parse --verify nope',
      )
      expect(
        await util.runGitAsync(repo, ['rev-parse', '--abbrev-ref', 'HEAD']),
      ).toBe('main\n')
    })

    it('runGclient points gclient at the file and honours verbose', () => {
      const run = jest
        .spyOn(util, 'run')
        .mockImplementation(() => ({}) as never)
      config.gclientVerbose = true
      util.runGclient(['sync'], { cwd: '/elsewhere' }, '/some/.gclient')

      const [cmd, args, options] = run.mock.calls[0]!
      expect(cmd).toBe('gclient')
      expect(args).toEqual(['sync', '--verbose'])
      expect(options!.cwd).toBe('/elsewhere')
      expect(options!.env!.GCLIENT_FILE).toBe('/some/.gclient')
    })

    it('runGclient defaults to the root checkout', () => {
      const run = jest
        .spyOn(util, 'run')
        .mockImplementation(() => ({}) as never)
      config.gclientVerbose = false
      config.gclientFile = '/default/.gclient'
      util.runGclient(['runhooks'])

      const [, args, options] = run.mock.calls[0]!
      expect(args).toEqual(['runhooks'])
      expect(options!.cwd).toBe(tmpDir)
      expect(options!.env!.GCLIENT_FILE).toBe('/default/.gclient')
    })

    it('massRename and launchDocs run their python tools', () => {
      const run = jest
        .spyOn(util, 'run')
        .mockImplementation(() => ({}) as never)
      util.massRename()
      util.launchDocs()

      expect(run.mock.calls[0]![0]).toBe('python3')
      expect(run.mock.calls[0]![1]![0]).toBe(
        path.join(config.srcDir, 'tools', 'git', 'mass-rename.py'),
      )
      expect(run.mock.calls[0]![2]!.cwd).toBe(config.braveCoreDir)
      expect(run.mock.calls[1]![0]).toBe('vpython3')
      expect(run.mock.calls[1]![1]![1]).toBe('brave/docs')
    })

    it('fetchAndCheckoutRef fetches the ref without its remote', () => {
      const run = jest
        .spyOn(util, 'run')
        .mockImplementation(() => ({}) as never)
      util.fetchAndCheckoutRef('/repo', 'origin/release')

      expect(run.mock.calls[0]![1]).toEqual(['fetch', 'origin', 'release'])
      expect(run.mock.calls[1]![1]).toEqual([
        '-c',
        'advice.detachedHead=false',
        'checkout',
        'FETCH_HEAD',
      ])
      expect(run.mock.calls[1]![2]).toMatchObject({ cwd: '/repo' })
    })
  })

  describe('getChangedFiles', () => {
    it('lists what differs from the merge base with the base branch', () => {
      const repo = path.join(tmpDir, 'repo')
      initRepo(repo)
      git(repo, 'checkout', '-q', '-b', 'feature')
      write(path.join(repo, 'added.txt'), 'new')
      fs.unlinkSync(path.join(repo, 'first.txt'))
      git(repo, 'add', '-A')
      git(repo, 'commit', '-q', '-m', 'change')
      // Unrelated work on the base branch does not count as a change.
      git(repo, 'checkout', '-q', 'main')
      write(path.join(repo, 'on_main.txt'), 'main')
      git(repo, 'add', '.')
      git(repo, 'commit', '-q', '-m', 'main')
      git(repo, 'checkout', '-q', 'feature')

      // Deleted files are left out.
      expect(util.getChangedFiles(repo, 'main', true)).toEqual(['added.txt'])
    })
  })

  describe('git exclusions', () => {
    const repo = () => path.join(tmpDir, 'repo')

    it('getGitDir finds the .git directory', () => {
      fs.mkdirSync(repo())
      expect(util.getGitDir(repo())).toBeNull()

      initRepo(repo())
      expect(util.getGitDir(repo())).toBe(path.join(repo(), '.git'))
    })

    it('getGitDir resolves the main .git from a worktree', () => {
      initRepo(repo())
      const worktree = path.join(tmpDir, 'worktree')
      git(repo(), 'worktree', 'add', '-q', '-b', 'other', worktree)

      expect(fs.realpathSync.native(util.getGitDir(worktree)!)).toBe(
        fs.realpathSync.native(path.join(repo(), '.git')),
      )
    })

    it('getGitDir handles a relative or missing common dir', () => {
      fs.mkdirSync(repo())
      write(path.join(repo(), '.git'), 'gitdir: elsewhere')
      const runGit = jest.spyOn(util, 'runGit').mockReturnValue('../main/.git')
      expect(util.getGitDir(repo())).toBe(path.join(repo(), '../main/.git'))

      runGit.mockReturnValue('')
      expect(util.getGitDir(repo())).toBeNull()
    })

    it('getGitInfoExcludeFileName finds or creates the file', () => {
      fs.mkdirSync(repo())
      expect(util.getGitInfoExcludeFileName(repo(), false)).toBeNull()
      expect(() => util.getGitInfoExcludeFileName(repo(), true)).toThrow(
        /\.git not found/,
      )

      initRepo(repo())
      const exclude = path.join(repo(), '.git', 'info', 'exclude')
      expect(util.getGitInfoExcludeFileName(repo(), false)).toBe(exclude)

      fs.rmSync(path.join(repo(), '.git', 'info'), { recursive: true })
      expect(util.getGitInfoExcludeFileName(repo(), false)).toBeNull()
      expect(util.getGitInfoExcludeFileName(repo(), true)).toBe(exclude)
      expect(fs.readFileSync(exclude, 'utf8')).toBe('')

      // The info directory may already be there without the file.
      fs.unlinkSync(exclude)
      expect(util.getGitInfoExcludeFileName(repo(), true)).toBe(exclude)
    })

    it('isGitExclusionExists looks for an exact line', () => {
      fs.mkdirSync(repo())
      expect(util.isGitExclusionExists(repo(), '/brave/')).toBe(false)

      initRepo(repo())
      const exclude = path.join(repo(), '.git', 'info', 'exclude')
      fs.writeFileSync(exclude, '# comment\r\n/brave/\r\nout/\n')
      expect(util.isGitExclusionExists(repo(), '/brave/')).toBe(true)
      expect(util.isGitExclusionExists(repo(), 'out/')).toBe(true)
      expect(util.isGitExclusionExists(repo(), 'brave')).toBe(false)
    })

    it('modifyGitExclusions adds and removes lines', () => {
      initRepo(repo())
      const exclude = path.join(repo(), '.git', 'info', 'exclude')
      fs.writeFileSync(exclude, 'keep\nold\n')

      util.modifyGitExclusions(repo(), {
        add: ['new', 'keep'],
        remove: ['old'],
      })
      expect(fs.readFileSync(exclude, 'utf8').split('\n')).toEqual([
        'keep',
        '',
        'new',
      ])
    })

    it('modifyGitExclusions has nothing to do without the file', () => {
      initRepo(repo())
      fs.rmSync(path.join(repo(), '.git', 'info'), { recursive: true })
      util.modifyGitExclusions(repo(), { remove: ['x'] })
      expect(fs.existsSync(path.join(repo(), '.git', 'info'))).toBe(false)

      // But adding creates it.
      util.modifyGitExclusions(repo(), { add: ['x'] })
      expect(
        fs.readFileSync(path.join(repo(), '.git', 'info', 'exclude'), 'utf8'),
      ).toBe('\nx')
    })
  })

  describe('gn', () => {
    const outputDir = () => path.join(config.srcDir, 'out', 'Release')
    // gn gen creates the output directory, runGnGen's guard does the same.
    beforeEach(() => fs.mkdirSync(outputDir(), { recursive: true }))

    it('writeGnBuildArgs writes the args and imports them from args.gn', () => {
      const updated = util.writeGnBuildArgs(outputDir(), {
        is_asan: true,
        brave_channel: 'nightly',
        target_cpu: undefined,
        'import("//brave/build/args/brave_defaults.gni")': null,
        list: ['a', 'b'],
      })

      expect(updated).toBe(true)
      const generated = fs.readFileSync(
        path.join(outputDir(), 'args_generated.gni'),
        'utf8',
      )
      expect(generated).toContain('# Do not edit')
      expect(generated).toContain('is_asan=true\n')
      expect(generated).toContain('brave_channel="nightly"\n')
      expect(generated).toContain(
        'import("//brave/build/args/brave_defaults.gni")\n',
      )
      expect(generated).toContain('list=["a","b"]\n')
      expect(generated).not.toContain('target_cpu')
      expect(
        fs.readFileSync(path.join(outputDir(), 'args.gn'), 'utf8'),
      ).toContain('import("//out/Release/args_generated.gni")')
      expect(Log.status).toHaveBeenCalledTimes(2)
    })

    it('writeGnBuildArgs reports no change on a repeat', () => {
      util.writeGnBuildArgs(outputDir(), { is_asan: true })
      expect(util.writeGnBuildArgs(outputDir(), { is_asan: true })).toBe(false)
      expect(util.writeGnBuildArgs(outputDir(), { is_asan: false })).toBe(true)
    })

    it('writeGnBuildArgs leaves user edits that keep the import alone', () => {
      util.writeGnBuildArgs(outputDir(), { is_asan: true })
      const argsGn = path.join(outputDir(), 'args.gn')
      const edited = `# import("//out/Release/args_generated.gni")\nmine=1\n`
      fs.writeFileSync(argsGn, edited)
      expect(util.writeGnBuildArgs(outputDir(), { is_asan: true })).toBe(false)
      expect(fs.readFileSync(argsGn, 'utf8')).toBe(edited)
    })

    it('writeGnBuildArgs rewrites an args.gn without the import', () => {
      util.writeGnBuildArgs(outputDir(), { is_asan: true })
      const argsGn = path.join(outputDir(), 'args.gn')
      fs.writeFileSync(argsGn, 'mine=1\n')
      expect(util.writeGnBuildArgs(outputDir(), { is_asan: true })).toBe(true)
      expect(fs.readFileSync(argsGn, 'utf8')).toContain(
        'import("//out/Release/args_generated.gni")',
      )
    })

    describe('runGnGen', () => {
      let run: jest.SpiedFunction<typeof util.run>
      beforeEach(() => {
        run = jest.spyOn(util, 'run').mockImplementation(() => ({}) as never)
        config.force_gn_gen = false
      })

      it('generates when there are no ninja files yet', () => {
        util.runGnGen(outputDir(), { is_asan: true }, ['--ide=json'])
        expect(run).toHaveBeenCalledWith(
          'gn',
          ['gen', outputDir(), '--ide=json', ...(isCI ? ['--check'] : [])],
          config.defaultOptions,
        )
        // Nothing is left marked as interrupted.
        expect(fs.existsSync(path.join(outputDir(), 'gn_gen.guard'))).toBe(
          false,
        )
        // The extra options are recorded so that changing them regenerates.
        expect(
          fs.readFileSync(path.join(outputDir(), 'args_generated.gni'), 'utf8'),
        ).toContain('# Extra gn gen options: --ide=json')
      })

      it('skips generating when nothing changed', () => {
        util.runGnGen(outputDir(), { is_asan: true })
        write(path.join(outputDir(), 'build.ninja'))
        run.mockClear()

        util.runGnGen(outputDir(), { is_asan: true })
        expect(run).toHaveBeenCalledTimes(isCI ? 1 : 0)
      })

      it('generates again when args change, or it is forced', () => {
        util.runGnGen(outputDir(), { is_asan: true })
        write(path.join(outputDir(), 'build.ninja'))
        run.mockClear()

        util.runGnGen(outputDir(), { is_asan: false })
        expect(run).toHaveBeenCalledTimes(1)

        config.force_gn_gen = true
        util.runGnGen(outputDir(), { is_asan: false })
        expect(run).toHaveBeenCalledTimes(2)
      })

      it('generates again after an interrupted run', () => {
        util.runGnGen(outputDir(), { is_asan: true })
        write(path.join(outputDir(), 'build.ninja'))
        write(path.join(outputDir(), 'gn_gen.guard'), 'interrupted')
        run.mockClear()

        util.runGnGen(outputDir(), { is_asan: true })
        expect(run).toHaveBeenCalledTimes(1)
      })

      it('leaves the guard behind when gn fails', () => {
        run.mockImplementation(() => {
          throw new Error('gn failed')
        })
        expect(() => util.runGnGen(outputDir(), { is_asan: true })).toThrow(
          'gn failed',
        )
        expect(fs.existsSync(path.join(outputDir(), 'gn_gen.guard'))).toBe(true)
      })
    })

    it('generateNinjaFiles generates in the output directory', async () => {
      const runGnGen = jest.spyOn(util, 'runGnGen').mockImplementation(() => {})
      config.extraGnGenOpts = '--ide=json'
      await util.generateNinjaFiles()
      expect(Log.progressScopeAsync).toHaveBeenCalledWith(
        'generate ninja files',
        expect.any(Function),
      )
      expect(runGnGen).toHaveBeenCalledWith(
        config.outputDir,
        expect.objectContaining({ is_asan: expect.any(Boolean) }),
        ['--ide=json'],
        expect.anything(),
      )

      config.extraGnGenOpts = ''
      await util.generateNinjaFiles()
      expect(runGnGen.mock.calls[1]![2]).toEqual([])
    })
  })

  describe('buildTargets', () => {
    let runAsync: jest.SpiedFunction<typeof util.runAsync>
    beforeEach(() => {
      runAsync = jest.spyOn(util, 'runAsync').mockResolvedValue('')
    })

    it('runs autoninja on the targets and marks the build finished', async () => {
      const outputDir = config.outputDir
      const sisoOutput = write(path.join(outputDir, 'siso_output'))

      await util.buildTargets(['brave', 'chrome'])

      expect(runAsync).toHaveBeenCalledTimes(1)
      const [cmd, args, options] = runAsync.mock.calls[0]!
      expect(cmd).toBe('autoninja')
      expect(args).toEqual(['-C', outputDir, 'brave', 'chrome', '-k', '1'])
      expect(options!.continueOnFail).toBe(true)
      expect(options!.env!.AUTONINJA_BUILD_ID).toMatch(/^[0-9a-f-]{36}$/)
      // A stale siso output is cleared and the guard does not linger.
      expect(fs.existsSync(sisoOutput)).toBe(false)
      expect(fs.existsSync(path.join(outputDir, 'build.guard'))).toBe(false)
      expect(Log.progressFinish).toHaveBeenCalled()
    })

    it('keeps going past compile failures when asked to', async () => {
      config.ignore_compile_failure = true
      await util.buildTargets(['brave'])
      expect(runAsync.mock.calls[0]![1]).toContain('0')
    })

    it('exits with the code of a failed build', async () => {
      const outputDir = config.outputDir
      runAsync.mockRejectedValue(
        Object.assign(new Error('build failed'), { statusCode: 1 }),
      )

      await expect(util.buildTargets(['brave'])).rejects.toThrow(
        'process.exit(1) called',
      )
      expect(consoleError).toHaveBeenCalledWith('build failed')
      // An expected build error is not an interrupted build.
      expect(fs.existsSync(path.join(outputDir, 'build.guard'))).toBe(false)
    })

    it('leaves the build marked as interrupted on other failures', async () => {
      const outputDir = config.outputDir
      runAsync.mockRejectedValue(
        Object.assign(new Error('killed'), { statusCode: 137 }),
      )

      await expect(util.buildTargets(['brave'])).rejects.toThrow(
        'process.exit(137) called',
      )
      expect(fs.existsSync(path.join(outputDir, 'build.guard'))).toBe(true)
    })

    it('generateXcodeWorkspace generates a separate Xcode output', () => {
      const runGnGen = jest.spyOn(util, 'runGnGen').mockImplementation(() => {})
      config.xcode_gen_target = '//brave/ios:*'
      util.generateXcodeWorkspace()

      expect(runGnGen).toHaveBeenCalledWith(
        `${config.outputDir}_Xcode`,
        expect.anything(),
        [
          '--ide=json',
          expect.stringContaining('xcode.py'),
          '--filters="//brave/ios:*"',
        ],
      )
    })
  })

  describe('touchOverriddenFiles', () => {
    const oldTime = 1000 // seconds
    let chromiumSrc: string
    beforeEach(() => {
      chromiumSrc = path.join(config.srcDir, 'brave', 'chromium_src')
      fs.mkdirSync(chromiumSrc, { recursive: true })
    })

    const mtime = (file: string) => fs.statSync(file).mtimeMs
    const makeOld = (file: string) => {
      write(file)
      fs.utimesSync(file, oldTime, oldTime)
      return file
    }

    it('touches originals older than their override', () => {
      const original = makeOld(path.join(config.srcDir, 'chrome', 'a.cc'))
      write(path.join(chromiumSrc, 'chrome', 'a.cc'))
      const newer = path.join(config.srcDir, 'chrome', 'b.cc')
      write(newer)
      write(path.join(chromiumSrc, 'chrome', 'b.cc'))
      fs.utimesSync(path.join(chromiumSrc, 'chrome', 'b.cc'), oldTime, oldTime)
      const unsupported = makeOld(path.join(config.srcDir, 'chrome', 'c.txt'))
      write(path.join(chromiumSrc, 'chrome', 'c.txt'))

      util.touchOverriddenFiles()

      expect(mtime(original)).toBeGreaterThan(oldTime * 1000)
      expect(mtime(newer)).toBeGreaterThan(oldTime * 1000)
      expect(mtime(unsupported)).toBe(oldTime * 1000)
      expect(consoleLog).toHaveBeenCalledWith(`${original} is touched.`)
      expect(Log.progressFinish).toHaveBeenCalled()
    })

    it('maps lit_mangler files onto the file they mangle', () => {
      const original = makeOld(path.join(config.srcDir, 'ui', 'page.html.ts'))
      write(path.join(chromiumSrc, 'ui', 'page.html.ts.lit_mangler.ts'))

      util.touchOverriddenFiles()

      expect(mtime(original)).toBeGreaterThan(oldTime * 1000)
    })

    it('deletes stale generated files of overrides without an original', () => {
      const gen = makeOld(path.join(config.outputDir, 'gen', 'ui', 'x.mojom'))
      const freshGen = path.join(config.outputDir, 'gen', 'ui', 'y.mojom')
      write(freshGen)
      write(path.join(chromiumSrc, 'ui', 'x.mojom'))
      write(path.join(chromiumSrc, 'ui', 'y.mojom'))
      fs.utimesSync(path.join(chromiumSrc, 'ui', 'y.mojom'), oldTime, oldTime)

      util.touchOverriddenFiles()

      expect(fs.existsSync(gen)).toBe(false)
      expect(fs.existsSync(freshGen)).toBe(true)
      expect(consoleLog).toHaveBeenCalledWith(`${gen} has been deleted.`)
    })

    it.each([
      ['android', 'arm64', 'android_clang_arm'],
      ['android', 'x64', 'android_clang_x86'],
      ['linux', 'arm64', 'clang_x64_v8_arm64'],
    ])(
      'also cleans the secondary gen dir of %s %s',
      (targetOS, targetArch, additionalGen) => {
        if (targetOS === 'linux' && process.platform === 'win32') {
          return
        }
        config.targetOS = targetOS
        config.targetArch = targetArch
        const secondary = makeOld(
          path.join(config.outputDir, additionalGen, 'gen', 'ui', 'x.mojom'),
        )
        write(path.join(chromiumSrc, 'ui', 'x.mojom'))

        util.touchOverriddenFiles()

        expect(fs.existsSync(secondary)).toBe(false)
      },
    )

    it('has no secondary gen dir for a plain host build', () => {
      config.targetArch = 'x64'
      const gen = makeOld(path.join(config.outputDir, 'gen', 'ui', 'x.mojom'))
      write(path.join(chromiumSrc, 'ui', 'x.mojom'))
      util.touchOverriddenFiles()
      expect(fs.existsSync(gen)).toBe(false)
    })

    it('clears the reproxy cache when an override changed', () => {
      config.rbeService = 'rbe.example.com:443'
      const cache = path.join(tmpDir, '.reproxy_cache')
      write(path.join(cache, 'a.cache'))
      write(path.join(cache, 'sub', 'b.cache.sha256'))
      write(path.join(cache, 'keep.txt'))
      makeOld(path.join(config.srcDir, 'chrome', 'a.cc'))
      write(path.join(chromiumSrc, 'chrome', 'a.cc'))

      util.touchOverriddenFiles()

      expect(fs.existsSync(path.join(cache, 'a.cache'))).toBe(false)
      expect(fs.existsSync(path.join(cache, 'sub', 'b.cache.sha256'))).toBe(
        false,
      )
      expect(fs.existsSync(path.join(cache, 'keep.txt'))).toBe(true)
    })

    it('keeps the reproxy cache when nothing changed', () => {
      config.rbeService = 'rbe.example.com:443'
      const cacheFile = write(path.join(tmpDir, '.reproxy_cache', 'a.cache'))
      util.touchOverriddenFiles()
      expect(fs.existsSync(cacheFile)).toBe(true)
    })

    it('does not look for the reproxy cache without RBE', () => {
      config.rbeService = ''
      const cacheFile = write(path.join(tmpDir, '.reproxy_cache', 'a.cache'))
      makeOld(path.join(config.srcDir, 'chrome', 'a.cc'))
      write(path.join(chromiumSrc, 'chrome', 'a.cc'))
      util.touchOverriddenFiles()
      expect(fs.existsSync(cacheFile)).toBe(true)
    })

    it('ignores a missing reproxy cache directory', () => {
      config.rbeService = 'rbe.example.com:443'
      makeOld(path.join(config.srcDir, 'chrome', 'a.cc'))
      write(path.join(chromiumSrc, 'chrome', 'a.cc'))
      expect(() => util.touchOverriddenFiles()).not.toThrow()
    })
  })
})
