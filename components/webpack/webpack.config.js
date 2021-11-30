// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

const path = require('path')
const fs = require('fs-extra')
const webpack = require('webpack')
const GenerateDepfilePlugin = require('./webpack-plugin-depfile')
const pathMap = require('./path-map')
const node_url = require("url");

const tsConfigPath = path.join(process.env.ROOT_GEN_DIR, 'tsconfig-webpack.json')



class AnySchemeUriPlugin {

    constructor(options = {}) {
        this.options = options;
    }

    apply(compiler) {
        compiler.hooks.compilation.tap(
            "AnySchemeUriPlugin",
            (compilation, { normalModuleFactory }) => {
                Array.from(this.options.schemes).forEach(scheme => {
                    normalModuleFactory.hooks.resolveForScheme
                        .for(scheme)
                        .tap("AnySchemeUriPlugin", resourceData => {
                            // const uri = resourceData.resource.replace(`${scheme}://`, 'file://');
                            const url = new node_url.URL(resourceData.resource);
                            const filepath = resourceData.resource.replace('chrome://resources', path.join(process.env.ROOT_GEN_DIR, 'ui/webui/resources/preprocessed')) + '.js'
                            const query = url.search;
                            const fragment = url.hash;
                            console.log(resourceData.resource, filepath, query, fragment)
                            resourceData.path = filepath;
                            resourceData.query = query;
                            resourceData.fragment = fragment;
                            resourceData.resource = filepath + query + fragment;
                            return true;
                        });
                });
            }
        );
    }
}

module.exports = AnySchemeUriPlugin;

module.exports = async function (env, argv) {
  // Webpack config object
  return {
    mode: argv.mode === 'development' ? 'development' : 'production',
    devtool: argv.mode === 'development' ? '#inline-source-map' : false,
    output: {
      path: process.env.TARGET_GEN_DIR,
      filename: '[name].bundle.js',
      chunkFilename: '[id].chunk.js'
    },
    resolve: {
      extensions: ['.js', '.tsx', '.ts', '.json'],
      alias: pathMap,
      // For explanation of "chromeapp", see:
      // https://github.com/brave/brave-browser/issues/5587
      aliasFields: ['chromeapp', 'browser'],
      fallback: {
        fs: false
      }
    },
    optimization: {
      // Define NO_CONCATENATE for analyzing module size.
      concatenateModules: !process.env.NO_CONCATENATE
    },
    plugins: process.env.DEPFILE_SOURCE_NAME ? [
      new GenerateDepfilePlugin({
        depfilePath: process.env.DEPFILE_PATH,
        depfileSourceName: process.env.DEPFILE_SOURCE_NAME
      }),
      new AnySchemeUriPlugin({schemes: ['chrome'] })
    ] : [],
    module: {
      rules: [
        {
          test: /\.tsx?$/,
          loader: 'ts-loader',
          exclude: /node_modules\/(?!brave-ui)/,
          options: {
            getCustomTransformers: path.join(__dirname, './webpack-ts-transformers.js'),
            allowTsInNodeModules: true,
            // Use generated tsconfig so that we can point at gen/ output in the
            // correct build configuration output directory.
            configFile: tsConfigPath
          }
        },
        {
          test: /\.css$/,
          use: ['style-loader', 'css-loader']
        },
        // Loads font files for Font Awesome
        {
          test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
          use: 'url-loader?limit=13000&minetype=application/font-woff'
        },
        {
          test: /\.(ttf|eot|ico|svg|png|jpg|jpeg|gif)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
          use: 'file-loader'
        }]
    }
  }
}
