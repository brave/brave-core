const config = require('./config')
const fs = require('fs-extra')
const path = require('path')
const readline = require('readline/promises')
const util = require('../lib/util')

const cleanWorkingTree = async (repoPath) => {
  console.log('Cleaning Chromium working tree...')
  await util.runGitAsync(repoPath, ['restore', '--staged', '.'])
  await util.runGitAsync(repoPath, ['checkout', '.'])
}

const updatePatch = async (repoPath, baseRevision, path, patchPath) => {
  const updatedPatch = await util.runGitAsync(repoPath,
      ['diff', baseRevision, '--src-prefix=a/', '--dst-prefix=b/', '--full-index', path])
  fs.writeFileSync(patchPath, updatedPatch)
}

const fixPatches = async (buildConfig = config.defaultBuildConfig, options) => {
  const GitPatcher = require('./gitPatcher')
  const coreRepoPath = config.braveCoreDir
  const patchesPath = path.join(coreRepoPath, 'patches')
  const chromiumRepoPath = config.srcDir
  const chromiumPatcher = new GitPatcher(patchesPath, chromiumRepoPath)
  // We assume that Chromium is already checked out at the correct revision.
  const chromiumRevision = (await util.runGitAsync(chromiumRepoPath, ['rev-parse', 'HEAD']))
      .replace(/(\r\n|\n|\r)/gm, '');

  async function RunCommand () {
    config.buildConfig = buildConfig
    config.update(options)

    await cleanWorkingTree(chromiumRepoPath)
    // Get standard apply results.
    const applyResults = await chromiumPatcher.applyPatches()
    await cleanWorkingTree(chromiumRepoPath)

    // Apply patches with 3-way merge.
    // TODO: only apply patches that failed with the initial apply
    const apply3WayResults = await chromiumPatcher.applyPatches3Way()
    for (const result of apply3WayResults) {
      if (!result.error) {
        const regularApplyResult = applyResults.find(res => res.path === result.path)
        // Only update patches which failed with standard apply here.
        if (regularApplyResult.error !== undefined) {
          await updatePatch(chromiumRepoPath, chromiumRevision, result.path, result.patchPath)
        }
      }
    }

    await cleanWorkingTree(chromiumRepoPath)
    await util.applyPatches()
  }

  RunCommand().catch((err) => {
    console.error(err)
    process.exit(1)
  })
}

module.exports = fixPatches
