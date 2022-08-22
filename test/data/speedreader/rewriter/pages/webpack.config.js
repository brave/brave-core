const { CleanWebpackPlugin } = require('clean-webpack-plugin')
const HtmlWebpackPlugin = require('html-webpack-plugin')
const path = require('path')

const isDevServer = process.argv[1].indexOf('webpack-dev-server') >= 0
const config = {
    mode: 'development',
    entry: __dirname + '/tools/report.js',
    output: {
        filename: '[contenthash].js',
        path: __dirname + '/report/docs',
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
            template: __dirname + '/tools/report.html',
        }),
    ],
    devServer: {
        open: true,
        port: 8028,
        static: {
            directory: path.join(__dirname, 'report'),
            publicPath: '/report',
         },
    },
}

if (!isDevServer) {
    config.plugins.push(new CleanWebpackPlugin())
}

module.exports = config
