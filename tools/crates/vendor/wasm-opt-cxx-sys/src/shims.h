#ifndef wasmopt_shims_h
#define wasmopt_shims_h

#include "pass.h"
#include "wasm-io.h"
#include "support/colors.h"
#include "wasm-validator.h"
#include "wasm-features.h"

#include <stdexcept> // runtime_error
#include <memory> // unique_ptr

namespace rust::behavior {
  template <typename Try, typename Fail>
  static void trycatch(Try&& func, Fail&& fail) noexcept try {
    func();
  } catch (const std::exception& e) {
    fail(e.what());
  } catch (const wasm::ParseException& e) {
    Colors::setEnabled(false);
    std::ostringstream buf;
    e.dump(buf);
    fail(buf.str());
  } catch (const wasm::MapParseException& e) {
    Colors::setEnabled(false);
    std::ostringstream buf;
    e.dump(buf);
    fail(buf.str());
  }
}

namespace wasm_shims {
  typedef wasm::Module Module;

  std::unique_ptr<Module> newModule() {
    return std::make_unique<Module>();
  }

  bool validateWasm(wasm::Module& wasm) {
    wasm::WasmValidator v;

    return v.validate(wasm);
  }
}

namespace wasm_shims {
  struct ModuleReader {
    wasm::ModuleReader inner;

    void setDebugInfo(bool debug) {
      inner.setDebugInfo(debug);
    }

    void setDwarf(bool dwarf) {
      inner.setDWARF(dwarf);
    }

    void readText(std::string& filename, Module& wasm) {
      inner.readText(std::move(filename), wasm);
    }

    void readBinary(std::string& filename,
                    Module& wasm,
                    std::string& sourceMapFilename) {
      inner.readBinary(std::move(filename),
                       wasm,
                       std::move(sourceMapFilename));
    }

    void read(std::string& filename,
              Module& wasm,
              std::string& sourceMapFilename) {
      inner.read(std::move(filename),
                 wasm,
                 std::move(sourceMapFilename));
    }
  };

  std::unique_ptr<ModuleReader> newModuleReader() {
    return std::make_unique<ModuleReader>();
  }
}

namespace wasm_shims {
  struct ModuleWriter {
    wasm::ModuleWriter inner;

    void setDebugInfo(bool debug) {
      inner.setDebugInfo(debug);
    }

    void setSourceMapFilename(std::string& source_map_filename) {
      inner.setSourceMapFilename(std::move(source_map_filename));
    }

    void setSourceMapUrl(std::string& source_map_url) {
      inner.setSourceMapUrl(std::move(source_map_url));
    }
  
    void writeText(Module& wasm,
                   std::string& filename) {
      inner.writeText(wasm, std::move(filename));
    }

    void writeBinary(Module& wasm,
                     std::string& filename) {
      inner.writeBinary(wasm, std::move(filename));
    }
  };
    
  std::unique_ptr<ModuleWriter> newModuleWriter() {
    return std::make_unique<ModuleWriter>();
  }
}

namespace wasm_shims {
  std::unique_ptr<std::vector<std::string>> getRegisteredNames() {
    auto r = wasm::PassRegistry::get();
    return std::make_unique<std::vector<std::string>>(r->getRegisteredNames());
  }

  std::unique_ptr<std::string> getPassDescription(std::string& name) {
    auto r = wasm::PassRegistry::get();
    return std::make_unique<std::string>(r->getPassDescription(std::move(name)));
  }

  bool isPassHidden(std::string& name) {
    auto r = wasm::PassRegistry::get();
    return r->isPassHidden(std::move(name));
  }
}

namespace wasm_shims {
  struct InliningOptions {
    wasm::InliningOptions inner;

    void setAlwaysInlineMaxSize(uint32_t size) {
      inner.alwaysInlineMaxSize = size;
    }

    void setOneCallerInlineMaxSize(uint32_t size) {
      inner.oneCallerInlineMaxSize = size;
    }

    void setFlexibleInlineMaxSize(uint32_t size) {
      inner.flexibleInlineMaxSize = size;
    }

    void setAllowFunctionsWithLoops(bool allow) {
      inner.allowFunctionsWithLoops = allow;
    }

    void setPartialInliningIfs(uint32_t number) {
      inner.partialInliningIfs = number;
    }
  };
    
  std::unique_ptr<InliningOptions> newInliningOptions() {
    return std::make_unique<InliningOptions>();
  }
}

namespace wasm_shims {
  struct PassOptions {
    wasm::PassOptions inner;

    void setValidate(bool validate) {
      inner.validate = validate;
    }

    void setValidateGlobally(bool validate) {
      inner.validateGlobally = validate;
    }

    void setOptimizeLevel(int32_t level) {
      inner.optimizeLevel = level;
    }

    void setShrinkLevel(int32_t level) {
      inner.shrinkLevel = level;
    }

    void setInliningOptions(std::unique_ptr<wasm_shims::InliningOptions> inlining) {
      inner.inlining = inlining->inner;
    }
    
    void setTrapsNeverHappen(bool ignoreTraps) {
      inner.trapsNeverHappen = ignoreTraps;
    }

    void setLowMemoryUnused(bool memoryUnused) {
      inner.lowMemoryUnused = memoryUnused;
    }

    void setFastMath(bool fastMath) {
      inner.fastMath = fastMath;
    }

    void setZeroFilledMemory(bool zeroFilledMemory) {
      inner.zeroFilledMemory = zeroFilledMemory;
    }

    void setDebugInfo(bool debugInfo) {
      inner.debugInfo = debugInfo;
    }

    void setArguments(std::string& key, std::string& value) {
      inner.arguments[std::move(key)] = std::move(value);
    }
  };

  std::unique_ptr<PassOptions> newPassOptions() {
    return std::make_unique<PassOptions>();
  }
}

namespace wasm_shims {
  struct WasmFeatureSet {
    wasm::FeatureSet inner;

    void setMVP() {
      inner.setMVP();
    }

    void setAll() {
      inner.setAll();
    }

    void set(uint32_t feature, bool val) {
      inner.set(feature, val);
    }

    bool has(const WasmFeatureSet& features) const {
      return inner.has(features.inner);
    }

    uint32_t as_int() const {
      return inner;
    }
  };

  std::unique_ptr<WasmFeatureSet> newFeatureSet() {
    return std::make_unique<WasmFeatureSet>();
  }

  std::unique_ptr<std::vector<uint32_t>> getFeatureArray() {
    std::vector<uint32_t> f;

    f.push_back(wasm::FeatureSet::Feature::None);
    f.push_back(wasm::FeatureSet::Feature::Atomics);
    f.push_back(wasm::FeatureSet::Feature::MutableGlobals);
    f.push_back(wasm::FeatureSet::Feature::TruncSat);
    f.push_back(wasm::FeatureSet::Feature::SIMD);
    f.push_back(wasm::FeatureSet::Feature::BulkMemory);
    f.push_back(wasm::FeatureSet::Feature::SignExt);
    f.push_back(wasm::FeatureSet::Feature::ExceptionHandling);
    f.push_back(wasm::FeatureSet::Feature::TailCall);
    f.push_back(wasm::FeatureSet::Feature::ReferenceTypes);
    f.push_back(wasm::FeatureSet::Feature::Multivalue);
    f.push_back(wasm::FeatureSet::Feature::GC);
    f.push_back(wasm::FeatureSet::Feature::Memory64);
    f.push_back(wasm::FeatureSet::Feature::RelaxedSIMD);
    f.push_back(wasm::FeatureSet::Feature::ExtendedConst);
    f.push_back(wasm::FeatureSet::Feature::Strings);
    f.push_back(wasm::FeatureSet::Feature::MultiMemory);
    // This is not part of the Rust API because it has the same value as None.
    // f.push_back(wasm::FeatureSet::Feature::MVP);
    f.push_back(wasm::FeatureSet::Feature::Default);
    f.push_back(wasm::FeatureSet::Feature::All);

    return std::make_unique<std::vector<uint32_t>>(f);
  }

  void applyFeatures(wasm::Module& wasm, std::unique_ptr<WasmFeatureSet> enabledFeatures, std::unique_ptr<WasmFeatureSet> disabledFeatures) {
    wasm.features.enable(enabledFeatures->inner);
    wasm.features.disable(disabledFeatures->inner);
  }
}

namespace wasm_shims {
  struct PassRunner {
    wasm::PassRunner inner;

    PassRunner(Module* wasm) : inner(wasm::PassRunner(wasm)) {}
    PassRunner(Module* wasm, PassOptions options) : inner(wasm::PassRunner(wasm, options.inner)) {}

    void add(std::string& passName) {
      inner.add(std::move(passName));
    }

    void addDefaultOptimizationPasses() {
      inner.addDefaultOptimizationPasses();
    }

    void run() {
      inner.run();
    }
  };

  std::unique_ptr<PassRunner> newPassRunner(Module& wasm) {
    return std::make_unique<PassRunner>(&wasm);
  }

  std::unique_ptr<PassRunner> newPassRunnerWithOptions(Module& wasm, std::unique_ptr<wasm_shims::PassOptions> options) {
    return std::make_unique<PassRunner>(&wasm, *options);
  }

  bool passRemovesDebugInfo(std::string& name) {
    return wasm::PassRunner::passRemovesDebugInfo(std::move(name));
  }
}

namespace wasm_shims {
  bool checkInliningOptionsDefaults(std::unique_ptr<InliningOptions> inlining) {
    auto inliningOptionsDefaults = wasm_shims::newInliningOptions();

    // The size assertion will fail when `InliningOptions` fields change,
    // which indicates the current test need to be updated.
    assert(sizeof(inliningOptionsDefaults->inner) == 20);
    
    bool isEqual = (inlining->inner.alwaysInlineMaxSize == inliningOptionsDefaults->inner.alwaysInlineMaxSize)
      && (inlining->inner.oneCallerInlineMaxSize == inliningOptionsDefaults->inner.oneCallerInlineMaxSize)
      && (inlining->inner.flexibleInlineMaxSize == inliningOptionsDefaults->inner.flexibleInlineMaxSize)
      && (inlining->inner.allowFunctionsWithLoops == inliningOptionsDefaults->inner.allowFunctionsWithLoops)
      && (inlining->inner.partialInliningIfs == inliningOptionsDefaults->inner.partialInliningIfs);
    
    return isEqual;
  }

  bool checkPassOptions(std::unique_ptr<PassOptions> passOptions, wasm::PassOptions passOptionsDefaults) {

    // The size assertion will fail when `PassOptions` or `InliningOptions` fields change.
    // We need to update the test when the size assertion failed.
    //    assert(sizeof(passOptionsDefaults) == 64); // Unix
    //    assert(sizeof(passOptionsDefaults) == 56); // Windows
    //    assert(sizeof(passOptionsDefaults) == 88); // Ubuntu
    // std::cout << " -------- size of optimizationsOptions " << sizeof(passOptionsDefaults);

    bool isEqual = (passOptions->inner.debug == passOptionsDefaults.debug)
      && (passOptions->inner.validate == passOptionsDefaults.validate)
      && (passOptions->inner.validateGlobally == passOptionsDefaults.validateGlobally)
      && (passOptions->inner.optimizeLevel == passOptionsDefaults.optimizeLevel) 
      && (passOptions->inner.shrinkLevel == passOptionsDefaults.shrinkLevel)
      && (passOptions->inner.trapsNeverHappen == passOptionsDefaults.trapsNeverHappen)
      && (passOptions->inner.lowMemoryUnused == passOptionsDefaults.lowMemoryUnused)
      && (passOptions->inner.fastMath == passOptionsDefaults.fastMath)
      && (passOptions->inner.zeroFilledMemory == passOptionsDefaults.zeroFilledMemory)
      && (passOptions->inner.debugInfo == passOptionsDefaults.debugInfo)
      // inlining fields comparison
      && (passOptions->inner.inlining.alwaysInlineMaxSize == passOptionsDefaults.inlining.alwaysInlineMaxSize)
      && (passOptions->inner.inlining.oneCallerInlineMaxSize == passOptionsDefaults.inlining.oneCallerInlineMaxSize)
      && (passOptions->inner.inlining.flexibleInlineMaxSize == passOptionsDefaults.inlining.flexibleInlineMaxSize)
      && (passOptions->inner.inlining.allowFunctionsWithLoops == passOptionsDefaults.inlining.allowFunctionsWithLoops)
      && (passOptions->inner.inlining.partialInliningIfs == passOptionsDefaults.inlining.partialInliningIfs);
   
    return isEqual;
  }

  bool checkPassOptionsDefaults(std::unique_ptr<PassOptions> passOptions) {
    auto passOptionsDefaults = wasm::PassOptions::getWithoutOptimization();

    return checkPassOptions(std::move(passOptions), passOptionsDefaults);
  }

  bool checkPassOptionsDefaultsOs(std::unique_ptr<PassOptions> passOptions) {
    auto passOptionsDefaults = wasm::PassOptions::getWithDefaultOptimizationOptions();

    return checkPassOptions(std::move(passOptions), passOptionsDefaults);
  }
}

#endif // wasmopt_shims_h

