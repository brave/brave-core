const path = require('path')
const fs = require('mz/fs')

function getIncludesString (idPrefix, fileList) {
  let includesString = ''
  for (const relativeFilePath of fileList) {
    const fileId = idPrefix + relativeFilePath.replace(/[^a-z0-9]/gi, '_').toUpperCase()
    includesString += `<include name="${fileId}" file="${relativeFilePath}" type="BINDATA" />
`
}
  return includesString
}


function getGrdString (name = 'brave_rewards_resources', idPrefix = 'IDR_BRAVE_REWARDS', fileList = []) {
  const includesString = getIncludesString(idPrefix, fileList)
  return `<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="0" current_release="1" output_all_resource_defines="false">
  <outputs>
    <output filename="grit/${name}_generated.h" type="rc_header">
      <emit emit_type='prepend'></emit>
    </output>
    <output filename="grit/${name}_generated_map.cc" type="resource_file_map_source" />
    <output filename="grit/${name}_generated_map.h" type="resource_map_header" />
    <output filename="${name}_generated.pak" type="data_package" />
  </outputs>
  <release seq="1">
    <includes>
      ${includesString}
    </includes>
  </release>
</grit>
`
}

// Returns Promise<string[]>
async function getFileListDeep (dirPath) {
  const dirItems = await fs.readdir(dirPath)
  // get Array<string | string[]> of contents
  const dirItemGroups = await Promise.all(dirItems.map(
    async (dirItemRelativePath) => {
      const itemPath = path.join(dirPath, dirItemRelativePath)
      const stats = await fs.stat(itemPath)
      if (stats.isDirectory()) {
        return await getFileListDeep(itemPath)
      }
      if (stats.isFile()) {
        return itemPath
      }
    }
  ))
  // flatten to single string[]
  return dirItemGroups.reduce(
    (flatList, dirItemGroup) => flatList.concat(dirItemGroup),
    []
  )
}

async function createDynamicGDR (name, idPrefix, targetDir) {
  // normalize path so relative path ignores leading path.sep
  if (!targetDir.endsWith(path.sep)) {
    targetDir += path.sep
  }
  const gdrPath = path.join(targetDir, `${name}.grd`)
  // remove previously generated file
  try {
    await fs.unlink(gdrPath)
  } catch (e) {}
  // build file list from target dir
  const filePaths = await getFileListDeep(targetDir)
  const relativeFilePaths = filePaths.map(filePath => filePath.replace(targetDir, ''))
  // get gdr string
  const gdrFileContents = getGrdString(name, idPrefix, relativeFilePaths)
  // write to file
  await fs.writeFile(gdrPath, gdrFileContents, { encoding: 'utf8' })
}

// collect args
const resourceName = process.env.RESOURCE_NAME
const idPrefix = process.env.ID_PREFIX
const targetDir = process.env.TARGET_DIR

if (!targetDir) {
  throw new Error("TARGET_DIR env variable is required!")
}
if (!idPrefix) {
  throw new Error("ID_PREFIX env variable is required!")
}
if (!resourceName) {
  throw new Error("RESOURCE_NAME env variable is required!")
}

// main
createDynamicGDR(resourceName, idPrefix, targetDir)
.catch(err => {
  console.error(err)
  process.exit(1)
})