// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs')
const config = require('../lib/config')
const util = require('../lib/util')

const gnSourcePath = path.join(config.braveCoreDir, 'third_party', 'gn')
const gnBuildPath = path.join(gnSourcePath, 'out')

async function gen() {
  const options = config.defaultOptions

  const clangBuildPath = path.join(
    config.srcDir,
    'third_party/llvm-build/Release+Asserts/bin',
  )

  if (config.hostOS === 'win') {
    options.env.CXX = path.join(clangBuildPath, 'clang-cl.exe')
    options.env.LD = path.join(clangBuildPath, 'lld-link.exe')
    options.env.AR = options.env.LD + ' /lib'

    const toolchainDirResult = util.run(
      'python3',
      [
        path.join(config.srcDir, 'build', 'vs_toolchain.py'),
        'get_toolchain_dir',
      ],
      { ...options, stdio: 'pipe' },
    )
    const vsPath = toolchainDirResult.stdout
      .toString()
      .trim()
      .match(/vs_path = "([^"]+)"/)[1]
      .replace(/\\\\/g, '/')

    if (vsPath.includes('depot_tools/win_toolchain')) {
      options.env.CXXFLAGS = `/winsysroot${vsPath}`
      options.env.LDFLAGS = `/winsysroot:${vsPath}`
    }
  } else {
    options.env.CXX = path.join(clangBuildPath, 'clang++')
    options.env.AR = path.join(clangBuildPath, 'llvm-ar')

    if (config.hostOS === 'mac') {
      const sysroot = `${config.srcDir}/build/mac_files/xcode_binaries/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`
      options.env.CXXFLAGS = `--sysroot=${sysroot}`
      options.env.LDFLAGS = `--sysroot=${sysroot}`
    }
    if (config.hostOS === 'linux') {
      options.env.CXXFLAGS = `-Wno-deprecated-declarations`
    }
  }

  util.run('python3', [path.join(gnSourcePath, 'build', 'gen.py')], options)
}

async function build() {
  const executable = util.appendExeIfWin32('gn')
  util.run('ninja', ['-C', gnBuildPath, executable], config.defaultOptions)

  const gnExecPath = path.join(gnBuildPath, executable)
  const buildtoolsPlatform =
    config.hostOS === 'linux' ? 'linux64' : config.hostOS
  const targetFilePath = path.join(
    config.srcDir,
    'buildtools',
    buildtoolsPlatform,
    executable,
  )
  if (fs.existsSync(targetFilePath)) {
    fs.unlinkSync(targetFilePath)
  }
  fs.copyFileSync(gnExecPath, targetFilePath)
}

async function test() {
  const executable = util.appendExeIfWin32('gn_unittests')
  util.run('ninja', ['-C', gnBuildPath, executable], config.defaultOptions)

  const gnTestPath = path.join(gnBuildPath, executable)
  util.run(gnTestPath, [], config.defaultOptions)
}

module.exports = {
  gen,
  build,
  test,
}
