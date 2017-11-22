const path = require('path')
const webpack = require('webpack')
const postCSSConfig = require('./postcss.config')

const customPath = path.join(__dirname, './customPublicPath')

module.exports = {
  entry: {
    braveShieldsPanel: [customPath, path.join(__dirname, '../app/braveShieldsPanel')],
    newTabPage: [customPath, path.join(__dirname, '../app/newTabPage')],
    background: [customPath, path.join(__dirname, '../app/background')]
  },
  output: {
    path: path.join(__dirname, '../build/js'),
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
    extensions: ['.js']
  },
  module: {
    rules: [{
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
