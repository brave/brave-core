solutions = [{
    "name": "src",
    "managed": False, 
    "url": "https://github.com/brave/chromium",
    "custom_deps": {
      "src/testing/libfuzzer/fuzzers/wasm_corpus": None,
      "src/third_party/chromium-variations": None
    },
    "custom_vars": {
      "checkout_pgo_profiles": False
    }
  }, {
    "name": "src/brave",
    "managed": False, 
    "url": "https://github.com/brave/brave-core.git"
  }
]

target_os = [ "android", "linux", "mac", "win" ]
target_cpu = [ "arm64", "x64"]