const path = require('path')

const isCI = Boolean(process.env.JENKINS_URL && process.env.BUILD_ID)

/** @type {import('@storybook/react-webpack5').StorybookConfig} */
module.exports = {
  stories: ['../components/**/stories/*.tsx', '../components/**/*.stories.tsx'],
  typescript: {
    check: true,
    reactDocgen: false,
    checkOptions: {
      async: !isCI,
      typescript: {
        configFile: path.resolve(__dirname, '..', 'tsconfig-storybook.json')
      }
    }
  },
  addons: ['@storybook/addon-knobs', '@storybook/addon-essentials'],
  framework: "@storybook/react-webpack5",
  staticDirs: [
    { from: '../node_modules/@brave/leo/icons', to: 'icons/' },
    {
      from: '../components/playlist/browser/resources/stories/assets',
      to: 'playlist/'
    }
  ],
  features: {
    storyStoreV7: false
  }
}
