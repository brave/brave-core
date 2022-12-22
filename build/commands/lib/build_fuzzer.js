const build = require('./build')

const build_fuzzer = (fuzzer_test_target, options) => {
  options.use_libfuzzer = true
  options.is_asan = true
  options.C = 'Fuzzer'
  options.target = fuzzer_test_target
  options.is_component_build = false
  options.is_debug = true

  build('Fuzzer', options)
}

module.exports = build_fuzzer
