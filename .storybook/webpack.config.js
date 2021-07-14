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
    'chrome://resources': path.resolve(__dirname, 'chrome-resources-mock'),
    'brave-ui': path.resolve(__dirname, '../node_modules/brave-ui/src'),
    // Force same styled-components module for brave-core and brave-ui
    // which ensure both repos code use the same singletons, e.g. ThemeContext.
    'styled-components': path.resolve(__dirname, '../node_modules/styled-components'),
  }
  config.resolve.extensions.push('.ts', '.tsx')
  return config
}
