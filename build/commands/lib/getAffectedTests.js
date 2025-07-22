// USAGE checkTest.js [COMMIT1 = HEAD^1] [COMMIT2 = HEAD]

const {promisify} = require('util');
const {readFile, writeFile} = require('fs/promises');
const exec = promisify(require('child_process').execFile);
const path = require('path')

const getTestTargets = (outDir = 'out/Component_arm64', filters=['//*']) => exec('gn', [
  'ls', 
  "../"+outDir, 
  '--type=executable', 
  '--testonly=true', 
  filters
]).then(x => x.stdout.trim().split('\n'));

// set base = HEAD if you want to ignore the current workspace changes
async function getModifiedFiles(target = "HEAD~", base = null) {
  const args = ['diff', '--name-only', target, base]
    .filter(x=>x)

  return await exec('git', args, { maxBuffer: 1024 * 1024 * 50 })
    .then(x => x.stdout.trim().split('\n')
    .filter(x => x)
    .map(x => '//brave/'+x));
}

async function getAffectedTests(outDir, filters = ['//*']) {
  // JENKINS sets GIT_PREVIOUS_SUCCESSFUL_COMMIT
  // TODO: find TeamCity equivalent.
  // TODO: we can optimize further by getting the last failure commit
  const ciLastSuccessfulCommit = process.env["GIT_PREVIOUS_SUCCESSFUL_COMMIT"];
  if (ciLastSuccessfulCommit) {
    console.log('Last Successful Commit', ciLastSuccessfulCommit)
  }
  const targetCommit = ciLastSuccessfulCommit || 'HEAD^';
//   const baseCommit = process.argv[3];

  const test_targets = await getTestTargets(outDir, filters);
  const files = await getModifiedFiles(targetCommit);

  const toAnalyze = {
    files,
    test_targets
  };

  // paths are relative to package.json
  const root = path.resolve(process.cwd(), '../');
  await writeFile(`${root}/out/analyze.json`, JSON.stringify(toAnalyze, null, 2), 'utf-8');
  await exec('gn', ['analyze', `${root}/${outDir}`, `${root}/out/analyze.json`, `${root}/out/out.json`]);
  const output = await readFile(`${root}/out/out.json`, 'utf-8')
    .then(JSON.parse)
  
  return {
    outDir,
    filters,
    ...toAnalyze,
    //baseCommit,
    targetCommit,
    affectedTests: output.test_targets
  };
}

module.exports = getAffectedTests;