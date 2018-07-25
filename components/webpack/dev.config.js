const path = require('path')
const webpack = require('webpack')

module.exports = {
  mode: 'development',
  devtool: '#inline-source-map',
  entry: {
    brave_adblock: path.join(__dirname, '../brave_adblock_ui/brave_adblock'),
    brave_new_tab: path.join(__dirname, '../brave_new_tab_ui/brave_new_tab'),
    brave_rewards: path.join(__dirname, '../brave_rewards/ui/brave_rewards'),
    brave_sync: path.join(__dirname, '../brave_sync_ui/brave_sync'),
    brave_welcome: path.join(__dirname, '../brave_welcome_ui/brave_welcome'),
    brave_webtorrent: path.join(__dirname, '../brave_webtorrent/extension/brave_webtorrent'),
    brave_webtorrent_background: path.join(__dirname, '../brave_webtorrent/extension/background')
  },
  output: {
    path: process.env.TARGET_GEN_DIR,
    filename: '[name].bundle.js',
    chunkFilename: '[id].chunk.js'
  },
  plugins: [
    new webpack.optimize.OccurrenceOrderPlugin(),
    new webpack.IgnorePlugin(/[^/]+\/[\S]+.dev$/)
  ],
  resolve: {
    extensions: ['.js', '.tsx', '.ts', '.json'],
    alias: {
      'bittorrent-tracker': path.resolve(__dirname, '../../node_modules/bittorrent-tracker'),
      'dgram': 'chrome-dgram',
      'dns': path.resolve(__dirname, '../common/dns.ts'),
      'net': 'chrome-net',
      'torrent-discovery': path.resolve(__dirname, '../../node_modules/torrent-discovery')
    }
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
        test: /\.(ttf|eot|svg|png|jpg|jpeg)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'file-loader'
      }]
  },
  node: {
    fs: 'empty'
  }
}
