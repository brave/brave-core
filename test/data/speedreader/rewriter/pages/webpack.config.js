/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CleanWebpackPlugin } from 'clean-webpack-plugin'
import HtmlWebpackPlugin from 'html-webpack-plugin'
import path from 'node:path'

const isDevServer = process.argv[1].indexOf('webpack-dev-server') >= 0
const config = {
    mode: 'development',
    entry: path.join(import.meta.dirname, '/tools/report.js'),
    output: {
        filename: '[contenthash].js',
        path: path.join(import.meta.dirname, '/report/docs'),
    },
    module: {
        rules: [
            {
                test: /\.(png|svg|jpg|gif)$/,
                use: [ 'file-loader' ],
            },
            {
                test: /\.html$/,
                use: [ 'html-loader' ],
            },
        ],
    },
    plugins: [
        new HtmlWebpackPlugin({
            template: path.join(import.meta.dirname, '/tools/report.html'),
        }),
    ],
    devServer: {
        open: true,
        port: 8028,
        static: {
            directory: path.join(import.meta.dirname, 'report'),
            publicPath: '/report',
         },
    },
}

if (!isDevServer) {
    config.plugins.push(new CleanWebpackPlugin())
}

export default config
