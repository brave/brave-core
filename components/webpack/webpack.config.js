const path = require('path')

module.exports = (env, argv) => ({
  devtool: argv.mode === 'development' ? '#inline-source-map' : false,
  output: {
    path: process.env.TARGET_GEN_DIR,
    filename: '[name].bundle.js',
    chunkFilename: '[id].chunk.js'
  },
  resolve: {
    extensions: ['.js', '.tsx', '.ts', '.json'],
    alias: {
      'bittorrent-tracker': path.resolve(__dirname, '../../node_modules/bittorrent-tracker'),
      'brave-ui': path.resolve(__dirname, '../../node_modules/brave-ui/src'),
      '~brave-core-ui': path.resolve(__dirname, '../../ui/webui/components'),
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
        test: /\.css$/,
        loader: ['style-loader', 'css-loader'],
      },
      // Loads font files for Font Awesome
      {
        test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'url-loader?limit=13000&minetype=application/font-woff'
      },
      {
        test: /\.(ttf|eot|ico|svg|png|jpg)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
        loader: 'file-loader'
      }]
  },
  node: {
    fs: 'empty'
  }
})
