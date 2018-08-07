const path = require('path')
const webpack = require('webpack')
const UglifyJSPlugin = require('uglifyjs-webpack-plugin')

module.exports = {
  mode: 'production',
  entry: {
    brave_adblock: path.join(__dirname, '../brave_adblock_ui/brave_adblock'),
    brave_new_tab: path.join(__dirname, '../brave_new_tab_ui/brave_new_tab'),
    brave_rewards: path.join(__dirname, '../brave_rewards_ui/brave_rewards'),
    brave_welcome: path.join(__dirname, '../brave_welcome_ui/brave_welcome'),
    brave_webtorrent: path.join(__dirname, '../../browser/resources/brave_webtorrent/brave_webtorrent'),
    brave_webtorrent_background: path.join(__dirname, '../../browser/resources/brave_webtorrent/background')
  },
  output: {
    path: process.env.TARGET_GEN_DIR,
    filename: '[name].bundle.js',
    chunkFilename: '[id].chunk.js'
  },
  plugins: [
    new webpack.optimize.OccurrenceOrderPlugin(),
    new webpack.IgnorePlugin(/[^/]+\/[\S]+.dev$/),
    new UglifyJSPlugin({
      uglifyOptions: {
        warnings: false,
        output: {
          comments: false
        },
        compress: {
          warnings: false
        }
      }
    })
  ],
  resolve: {
    extensions: ['.js', '.tsx', '.ts', '.json'],
    alias: {
      'dgram': 'chrome-dgram',
      'net': 'chrome-net'
    }
  },
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        loader: 'ts-loader'
      },
      {
        test: /\.js$/,
        loader: 'babel-loader',
        exclude: /node_modules/,
        query: {
          presets: ['react-optimize']
        }
      },
      {
        test: /\.less$/,
        loader: 'style-loader!css-loader?-minimize!less-loader'
      },
      {
        test: /\.css$/,
        loader: 'style-loader!css-loader?-minimize'
      },
      // Loads font files for Font Awesome
      {
        test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'url-loader?limit=13000&minetype=application/font-woff'
      },
      {
        test: /\.(ttf|eot|svg|png|jpg)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'file-loader'
      }]
  },
  node: {
    fs: 'empty'
  }
}
