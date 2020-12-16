const path = require('path')

// Export a function. Accept the base config as the only param.
module.exports = async ({ config, mode }) => {
  // Make whatever fine-grained changes you need
  config.module.rules.push({
    test: /\.(ts|tsx)$/,
    loader: require.resolve('ts-loader'),
    exclude: /node_modules\/(?!brave-ui)/,
    options: {
      configFile: path.resolve(__dirname, '..', 'tsconfig-storybook.json'),
      allowTsInNodeModules: true,
      getCustomTransformers: path.join(__dirname, '../components/webpack/webpack-ts-transformers.js'),
    }
  })
  config.module.rules.push({
    test: /\.avif$/,
    loader: 'file-loader'
  })
  config.resolve.alias = {
    ...config.resolve.alias,
    'brave-ui': path.resolve(__dirname, '../node_modules/brave-ui/src')
  }
  config.resolve.extensions.push('.ts', '.tsx')
  return config
}
