const Path = require('path')

const storyLocations = [
  '**/stories/*.tsx',
  '**/*.stories.tsx'
]

function getStories () {
  if (process.env['STORYBOOK_STORY_PATH']) {
    return process.env['STORYBOOK_STORY_PATH']
      .split(/\s+/)
      .flatMap((storyPath) => storyLocations.map((location) => {
        return Path.resolve(storyPath, location)
      }))
  }

  return storyLocations.map((location) => {
    return Path.join('../components/', location)
  })
}

module.exports = {
  stories: getStories(),
  addons: [
    '@storybook/addon-knobs',
    '@storybook/addon-essentials'
  ]
}
