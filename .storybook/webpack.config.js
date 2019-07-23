const path = require('path')

const createStyledComponentsTransformer = require('typescript-plugin-styled-components')
  .default

function getStyledComponentDisplay (filename, bindingName) {
  return bindingName
}

// Export a function. Accept the base config as the only param.
module.exports = async ({ config, mode }) => {
  // Make whatever fine-grained changes you need
  config.module.rules.push({
    test: /\.(ts|tsx)$/,
    loader: require.resolve('awesome-typescript-loader'),
    options: {
      configFileName: path.resolve(__dirname, '..', 'tsconfig-storybook.json'),
      getCustomTransformers: () => ({
        before: [
          createStyledComponentsTransformer({
            options: {
              getDisplayName: getStyledComponentDisplay
            }
          })
        ]
      })
    }
  })
  config.resolve.alias = {
    ...config.resolve.alias,
    'brave-ui': path.resolve(__dirname, '../node_modules/brave-ui/src')
  }
  config.resolve.extensions.push('.ts', '.tsx')
  return config
}
