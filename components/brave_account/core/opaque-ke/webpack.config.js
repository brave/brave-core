const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const webpack = require('webpack');
const WasmPackPlugin = require("@wasm-tool/wasm-pack-plugin");

module.exports = (env) => {
  // WasmPackPlugin expects wasm-pack to be in $PATH,
  // so prepend ours to process.env.PATH (so that it's found first)
  process.env.PATH = env.wasm_pack_path + path.delimiter + process.env.PATH

  return {
    context: path.resolve(__dirname, '.'),
    entry: './src/index.js',
    resolve: {
      alias: {
        pkg: path.resolve(env.output_path, 'pkg'),
      },
    },
    output: {
      clean: true,
      filename: 'opaque_ke.bundle.js',
      library: {
        type: 'module',
      },
      module: true,
      path: path.resolve(env.output_path, 'dist'),
      publicPath: 'chrome://resources/brave/opaque-ke/bundle/',
      webassemblyModuleFilename: 'opaque_ke.module.wasm'
      // trustedTypes: true,
    },
    plugins: [
      new HtmlWebpackPlugin(),
      new WasmPackPlugin({
        crateDirectory: path.resolve(__dirname, "."),
        outName: 'opaque_ke',
        outDir: path.resolve(env.output_path, 'pkg'),
        extraArgs: `--target-dir ${path.resolve(env.output_path, 'target')}`
      }),
    ],
    devtool: 'source-map',  // suppressing generating eval() calls
    mode: 'development',
    experiments: {
      asyncWebAssembly: true,
      outputModule: true,
    }
  }
};
