module.exports = {
  stories: ['../components/**/stories/*.tsx'],
  addons: [
    '@storybook/addon-knobs',
    '@storybook/addon-essentials'
  ],
  core: {
    builder: 'webpack5'
  }
}