// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { readFile, writeFile } from 'fs/promises'
import { createHash } from 'crypto'
import { fileURLToPath } from 'url'
import { dirname, join } from 'path'

const __filename = fileURLToPath(import.meta.url)
const __dirname = dirname(__filename)
const rootDir = join(__dirname, '..', '..')

// All supported platforms
const PLATFORMS = [
  'opengrep_osx_arm64',
  'opengrep_osx_x86',
  'opengrep_manylinux_x86',
  'opengrep_manylinux_aarch64',
  'opengrep_musllinux_x86',
  'opengrep_musllinux_aarch64',
  'opengrep_windows_x86.exe',
]

async function fetchLatestRelease() {
  console.log('Fetching latest opengrep release from GitHub...')
  const response = await fetch(
    'https://api.github.com/repos/opengrep/opengrep/releases/latest',
  )

  if (!response.ok) {
    throw new Error(
      `Failed to fetch releases: ${response.status} ${response.statusText}`,
    )
  }

  const release = await response.json()
  const version = release.tag_name

  if (!version) {
    throw new Error('Could not find version tag in release data')
  }

  console.log(`Latest release: ${version}`)
  return version
}

async function downloadBinary(version, platform) {
  const url = `https://github.com/opengrep/opengrep/releases/download/${version}/${platform}`
  console.log(`  Downloading ${platform}...`)

  const response = await fetch(url)

  if (!response.ok) {
    console.log(
      `    Warning: Failed to download ${platform} (${response.status})`,
    )
    return null
  }

  const buffer = await response.arrayBuffer()
  return Buffer.from(buffer)
}

function calculateSHA256(buffer) {
  const hash = createHash('sha256')
  hash.update(buffer)
  return hash.digest('hex')
}

async function downloadAndHashBinaries(version) {
  console.log('\nDownloading binaries and calculating checksums...')

  const checksums = {}
  let downloadedCount = 0

  for (const platform of PLATFORMS) {
    const buffer = await downloadBinary(version, platform)

    if (buffer) {
      const checksum = calculateSHA256(buffer)
      checksums[platform] = checksum
      console.log(`  ✓ ${platform}: ${checksum}`)
      downloadedCount++
    }
  }

  if (downloadedCount === 0) {
    throw new Error(
      'No binaries could be downloaded. Check the version number.',
    )
  }

  console.log(`\n✓ Downloaded ${downloadedCount}/${PLATFORMS.length} binaries`)
  return checksums
}

async function updateInstallScript(version, checksums) {
  const installScriptPath = join(rootDir, 'third_party/opengrep/install_opengrep.py')
  console.log(`\nUpdating ${installScriptPath}...`)

  let content = await readFile(installScriptPath, 'utf-8')
  let hasChanges = false

  // Update version
  const versionRegex = /^OPENGREP_VERSION = '[^']*'/m
  const newVersionLine = `OPENGREP_VERSION = '${version}'`

  if (content.match(versionRegex)) {
    const oldContent = content
    content = content.replace(versionRegex, newVersionLine)
    if (oldContent !== content) {
      console.log(`  ✓ Updated OPENGREP_VERSION to '${version}'`)
      hasChanges = true
    }
  }

  // Update checksums in BINARY_CHECKSUMS dictionary
  // The format is:
  //     'platform': (
  //         'checksum'),
  for (const [platform, checksum] of Object.entries(checksums)) {
    // Escape special regex characters in platform name (for .exe)
    const escapedPlatform = platform.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')

    // Match multi-line pattern: 'platform': (\n        'checksum'),
    const checksumRegex = new RegExp(
      `('${escapedPlatform}': \\(\\n\\s+')[a-f0-9]{64}('\\),)`,
      'gm',
    )

    if (content.match(checksumRegex)) {
      const oldContent = content
      content = content.replace(checksumRegex, `$1${checksum}$2`)
      if (oldContent !== content) {
        console.log(`  ✓ Updated ${platform}`)
        hasChanges = true
      }
    } else {
      console.log(`  - Warning: Could not find checksum entry for ${platform}`)
    }
  }

  if (hasChanges) {
    await writeFile(installScriptPath, content, 'utf-8')
    console.log('\n✓ Successfully updated install_opengrep.py')
    return true
  } else {
    console.log('\n- No changes needed')
    return false
  }
}

async function verifyPythonSyntax() {
  console.log('\nVerifying Python syntax...')
  const installScriptPath = join(rootDir, 'third_party/opengrep/install_opengrep.py')

  try {
    const { exec } = await import('child_process')
    const { promisify } = await import('util')
    const execAsync = promisify(exec)

    await execAsync(`python3 -m py_compile "${installScriptPath}"`)
    console.log('✓ Python syntax validation passed')
    return true
  } catch (error) {
    console.error('Error: Python syntax validation failed')
    console.error(error.message)
    return false
  }
}

async function main() {
  try {
    // Fetch latest release version
    const version = await fetchLatestRelease()

    // Check if already up to date
    const installScriptPath = join(rootDir, 'third_party/opengrep/install_opengrep.py')
    const currentContent = await readFile(installScriptPath, 'utf-8')
    const currentVersionMatch = currentContent.match(
      /OPENGREP_VERSION = '([^']+)'/,
    )

    if (currentVersionMatch && currentVersionMatch[1] === version) {
      console.log(`\n✓ Opengrep is already up to date (${version})`)
      process.exit(0) // Exit with code 0 for success (no changes needed)
    }

    // Download binaries and calculate checksums
    const checksums = await downloadAndHashBinaries(version)

    // Update install script
    const hasChanges = await updateInstallScript(version, checksums)

    if (!hasChanges) {
      console.log('\n✓ No changes were made')
      process.exit(0)
    }

    // Verify Python syntax
    const syntaxValid = await verifyPythonSyntax()

    if (!syntaxValid) {
      console.error('\nError: Python syntax validation failed')
      process.exit(1)
    }

    console.log('\n===========================================')
    console.log('✓ Successfully updated opengrep')
    console.log('===========================================')
    console.log(`\nVersion: ${version}`)
    console.log(`Updated: ${Object.keys(checksums).length} checksums`)
    console.log('\nChanges:')
    console.log('  - third_party/opengrep/install_opengrep.py')

    process.exit(0)
  } catch (error) {
    console.error('\nError updating opengrep:', error.message)
    if (error.stack) {
      console.error(error.stack)
    }
    process.exit(1)
  }
}

main()
