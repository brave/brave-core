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
    // Put mojom-generated output (either copied from real generated
    // output or manually create modules exporting expected types) so
    // that storybook UI may use them. This is preferred to be copied here so that
    // 1. Storybook can be compiled without the browser being compiled,
    // and 2. To act as a snapshot.
    'gen': path.resolve(__dirname, 'gen-mock'),
    // If stories include calls to chromium code, the functions should be mocked.
    'chrome://resources': path.resolve(__dirname, 'chrome-resources-mock'),
    'brave-ui': path.resolve(__dirname, '../node_modules/brave-ui/src'),
    // Force same styled-components module for brave-core and brave-ui
    // which ensure both repos code use the same singletons, e.g. ThemeContext.
    'styled-components': path.resolve(__dirname, '../node_modules/styled-components'),
  }
  config.resolve.extensions.push('.ts', '.tsx')
  return config
}
