module.exports = {
  stories: [
    '../components/**/stories/*.tsx',
    '../components/**/*.stories.tsx'
  ],
  addons: [
    '@storybook/addon-knobs',
    '@storybook/addon-essentials'
  ],
  staticDirs: [
    { from: '../node_modules/@brave/leo/icons', to: 'icons/' },
    {
      from: '../components/playlist/browser/resources/stories/assets',
      to: 'playlist/'
    }
  ]
}
