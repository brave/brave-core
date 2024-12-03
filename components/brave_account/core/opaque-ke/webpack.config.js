const path = require('path');
const WasmPackPlugin = require("@wasm-tool/wasm-pack-plugin");
const { XHRCompileAsyncWasmPlugin } = require('./xhr_compile_async_wasm_plugin.js');

module.exports = (env, argv) => {
  // WasmPackPlugin expects wasm-pack to be in $PATH,
  // so prepend ours to process.env.PATH (so that it's found first).
  process.env.PATH = env.wasm_pack_path + path.delimiter + process.env.PATH

  return {
    context: path.resolve(__dirname, '.'),
    entry: {
      opaque_ke: './src/index.js',
    },
    resolve: {
      alias: {
        pkg: path.resolve(env.output_path, 'pkg'),
      },
    },
    output: {
      clean: true,
      filename: '[name].bundle.js',
      library: {
        type: 'module',
      },
      module: true,
      path: path.resolve(env.output_path, 'dist'),
      webassemblyModuleFilename: 'opaque_ke.module.wasm',
      enabledWasmLoadingTypes: [ "xhr" ],
      wasmLoading: "xhr",
    },
    plugins: [
      new WasmPackPlugin({
        crateDirectory: path.resolve(__dirname, "."),
        extraArgs: `--target-dir ${path.resolve(env.output_path, 'target')}`,
        forceMode: argv.mode,
        outDir: path.resolve(env.output_path, 'pkg'),
        outName: 'opaque_ke',
      }),
      new XHRCompileAsyncWasmPlugin(),
    ],
    devtool: argv.mode === 'development' ? 'inline-source-map' : false,
    experiments: {
      asyncWebAssembly: true,
      outputModule: true,
    }
  }
};
