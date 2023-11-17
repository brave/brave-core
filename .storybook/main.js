const path = require('path')

module.exports = {
  stories: ['../components/**/stories/*.tsx', '../components/**/*.stories.tsx'],
  typescript: {
    check: true,
    checkOptions: {
      tsconfig: path.resolve(__dirname, '..', 'tsconfig-storybook.json'),
      compilerOptions: {
        allowJs: true,
        skipLibCheck: true,
        noImplicitAny: true,
        outDir: './storybook_dist'
      },
      async: false
    }
  },
  addons: ['@storybook/addon-knobs', '@storybook/addon-essentials'],
  staticDirs: [
    { from: '../node_modules/@brave/leo/icons', to: 'icons/' },
    {
      from: '../components/playlist/browser/resources/stories/assets',
      to: 'playlist/'
    }
  ]
}
