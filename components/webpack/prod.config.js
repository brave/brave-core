const path = require('path')
const webpack = require('webpack')
const postCSSConfig = require('./postcss.config')

module.exports = {
  entry: {
    brave_new_tab: path.join(__dirname, '../brave_new_tab_ui/brave_new_tab'),
    welcome: path.join(__dirname, '../welcome_ui/welcome'),
    brave_payments: path.join(__dirname, '../brave_payments_ui/brave_payments')
  },
  output: {
    path: path.join(__dirname, '..', '..', '..', process.env.TARGET_GEN_DIR, 'brave'),
    filename: '[name].bundle.js',
   chunkFilename: '[id].chunk.js'
  },
  plugins: [
    new webpack.optimize.OccurrenceOrderPlugin(),
    new webpack.IgnorePlugin(/[^/]+\/[\S]+.dev$/),
    new webpack.optimize.UglifyJsPlugin({
      comments: false,
      compressor: {
        warnings: false
      }
    }),
    new webpack.DefinePlugin({
      'process.env': {
        NODE_ENV: JSON.stringify('production')
      }
    })
  ],
  resolve: {
    extensions: ['.ts', '.tsx', '.js']
  },
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        loader: 'awesome-typescript-loader'
      },
      {
        test: /\.js$/,
        loader: 'babel-loader',
        exclude: /node_modules/,
        query: {
          presets: ['react-optimize']
        }
      }, {
        test: /\.css$/,
        loaders: [
          'style-loader',
          'css-loader?modules&importLoaders=1&localIdentName=[name]__[local]___[hash:base64:5]',
          {loader: 'postcss-loader', options: postCSSConfig}
        ]
      }]
  }
}
