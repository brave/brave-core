const path = require('path')
const AliasPlugin = require('enhanced-resolve/lib/AliasPlugin')

// Export a function. Accept the base config as the only param.
module.exports = async ({ config, mode }) => {
  // Make whatever fine-grained changes you need
  config.module.rules.push({
    test: /\.(ts|tsx)$/,
    loader: require.resolve('ts-loader'),
    exclude: /node_modules\/(?!brave-ui)/,
    options: {
      // TODO(petemill): point to the tsconfig in gen/[target] that
      // is made during build-time, or generate a new one. For both those
      // options, use a cli arg or environment variable to obtain the correct
      // build target.
      configFile: path.resolve(__dirname, '..', 'tsconfig-storybook.json'),
      allowTsInNodeModules: true,
      getCustomTransformers: path.join(__dirname, '../components/webpack/webpack-ts-transformers.js'),
    }
  })
  config.module.rules.push({
    test: /\.avif$/,
    loader: 'file-loader'
  })
  // Use webpack resolve.alias from v5 whilst still using webpack v4,
  // so that we can use an array as value
  config.resolve.plugins = [
    ...config.resolve.plugins,
    new AliasPlugin(
      'described-resolve', [
        {
          name: 'brave-ui',
          alias: path.resolve(__dirname, '../node_modules/brave-ui/src'),
        },
        {
          // Force same styled-components module for brave-core and brave-ui
          // which ensure both repos code use the same singletons, e.g. ThemeContext.
          name: 'styled-components',
          alias: path.resolve(__dirname, '../node_modules/styled-components'),
        },
        {
          name: 'chrome://resources',
          alias: [
            // Mock chromium code where possible
            path.resolve(__dirname, 'chrome-resources-mock'),
            // TODO(petemill): This is not ideal since if the dev is using a Static
            // build, but has previously made a Component build, then an outdated
            // version of a module will be used. Instead, accept a cli argument
            // or environment variable containing which build target to use.
            path.resolve(__dirname, '../../out/Component/gen/ui/webui/resources'),
            path.resolve(__dirname, '../../out/Static/gen/ui/webui/resources'),
            path.resolve(__dirname, '../../out/Debug/gen/ui/webui/resources'),
            path.resolve(__dirname, '../../out/Release/gen/ui/webui/resources'),
          ]
        },
        {
          name: 'gen',
          alias: [
            // Mock chromium code where possible
            path.resolve(__dirname, 'gen-mock'),
            // TODO(petemill): This is not ideal since if the dev is using a Static
            // build, but has previously made a Component build, then an outdated
            // version of a module will be used. Instead, accept a cli argument
            // or environment variable containing which build target to use.
            path.resolve(__dirname, '../../out/Component/gen'),
            path.resolve(__dirname, '../../out/Static/gen'),
            path.resolve(__dirname, '../../out/Debug/gen'),
            path.resolve(__dirname, '../../out/Release/gen'),
          ]
        },
      ], 'resolve'
    )
  ]
  config.resolve.extensions.push('.ts', '.tsx')
  return config
}
