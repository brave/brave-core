const path = require('path')

const isCI = Boolean(process.env.JENKINS_URL && process.env.BUILD_ID)

module.exports = {
  stories: ['../components/**/stories/*.tsx', '../components/**/*.stories.tsx'],
  typescript: {
    check: true,
    checkOptions: {
      tsconfig: path.resolve(__dirname, '..', 'tsconfig-storybook.json'),
      compilerOptions: {
        'react-docgen-typescript': false,
        allowJs: true,
        skipLibCheck: true,
        noImplicitAny: true,
        outDir: './storybook_dist'
      },
      async: !isCI
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
  },
  typescript: {
    check: false,
    reactDocgen: false
  }
}
