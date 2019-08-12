const path = require('path')
const GenerateDepfilePlugin = require('./webpack-plugin-depfile')

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
      'brave-ui': path.resolve(__dirname, '../../node_modules/brave-ui/src')
    },
    // For explanation of "chromeapp", see:
    // https://github.com/brave/brave-browser/issues/5587
    aliasFields: ['chromeapp']
  },
  plugins: [
    new GenerateDepfilePlugin({
      depfilePath: process.env.DEPFILE_PATH,
      depfileSourceName: process.env.DEPFILE_SOURCE_NAME
    })
  ],
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        loader: 'awesome-typescript-loader'
      },
      {
        test: /\.css$/,
        loader: ['style-loader', 'css-loader']
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
