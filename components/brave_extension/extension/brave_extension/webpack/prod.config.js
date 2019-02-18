const path = require('path')
const webpack = require('webpack')
const postCSSConfig = require('./postcss.config')
const UglifyJSPlugin = require('uglifyjs-webpack-plugin')

const customPath = path.join(__dirname, './customPublicPath')

module.exports = {
  entry: {
    braveShieldsPanel: [customPath, path.join(__dirname, '../app/braveShieldsPanel')],
    background: [customPath, path.join(__dirname, '../app/background')],
    content: [customPath, path.join(__dirname, '../app/content')],
  },
  mode: 'production',
  optimization: {
    minimizer: [
      new UglifyJSPlugin({
        uglifyOptions: {
          comments: false,
          compressor: {
            warnings: false
          }
        }
      })
    ]
  },
  output: {
    path: path.join(process.env.TARGET_GEN_DIR, 'js'),
    filename: '[name].bundle.js',
    chunkFilename: '[id].chunk.js'
  },
  plugins: [
    new webpack.optimize.OccurrenceOrderPlugin(),
    new webpack.IgnorePlugin(/[^/]+\/[\S]+.dev$/),
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
          { loader: 'postcss-loader', options: postCSSConfig }
        ]
      },
      {
        test: /\.(woff(2)|svg)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'url-loader?limit=13000&minetype=application/font-woff'
      },
      {
        test: /\.(ttf|eot|ico|png|jpg|jpeg)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'file-loader'
      }]
  }
}
