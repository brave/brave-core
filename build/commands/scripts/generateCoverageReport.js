const {glob} =require('fs/promises');
const {writeJSON} = require('fs-extra')
const utils = require('../lib/util')
const config = require('../lib/config')

module.exports = (program) =>
  program
    .command('coverage_report')
    .option('-C [build_dir]', 'build config (out/Debug, out/Release)')
    .option('--build_type [build_type]', 'build with coverage information ')
    .option('--target_arch [target_arch]', 'target architecture')
    .option('--target_os <target_os>', 'target OS')
    .option('--tests [testSuites...]', 'testsuites to consider')
    .description('generates a coverage report')
    .action(async (args) => {
      if (args.buildConfig) {
        config.buildConfig = args.buildConfig
      }
      config.update(args)

      const out = `${config.outputDir}/coverage`;
      const recordings = await Array.fromAsync(glob(`${out}/**/*.profraw`))

      const testSuites = [...new Set(recordings
        .map(x =>x.replace(out+'/',''))
        .map(x => x.split('/'))
        .filter(x => x.length > 1)
        .map(x => x[0]))
      ] // seems like some binaries cause the html report to fail but not the json export?!
      .filter( x => x.includes('brave_browser_tests') )
      .map(x => `${config.outputDir}/${x}`)
      

      if (!recordings.length) {
        console.error(`glob ${out}/**/*.profraw yield any recordings!`)
        return;
      }


      process.env.CWD = config.outputDir
      process.env.PATH = `${process.env.PATH}:${config.srcDir}/third_party/llvm-build/Release+Asserts/bin`
      // fetch coverage tools if not available
      await utils.runAsync("python3", [`${config.srcDir}/tools/clang/scripts/update.py`, "--package=coverage_tools"])
      await utils.runAsync("llvm-profdata", ["merge", "-sparse", "-o", `${out}/coverage.profdata`, ...recordings])
      await utils.runAsync("llvm-cov", [
        "show", 
        `-compilation-dir=${config.outputDir}`,
        `-instr-profile=${out}/coverage.profdata`,
        "-format=html",
        `-output-dir=${out}/report`,
        ...testSuites
      ]);

      const output = await utils.runAsync("llvm-cov", [
        "export", 
        `-compilation-dir=${config.outputDir}`,
        `-instr-profile=${out}/coverage.profdata`,
        "--summary-only",
        ...testSuites
      ], {stdio: 'pipe'});

      try {
        const summary = JSON.parse(output);
        await writeJSON(`${out}/report/coverage.json`, summary);
        console.log(summary.data[0].totals);
      } catch (e) {
        console.error(e)
      }

      console.log(`\ncoverage reports written to ${out}/report`)
    })