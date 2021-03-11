import { addons } from '@storybook/addons'
import { create } from '@storybook/theming'

const braveTheme = create({
  base: 'dark',
  brandTitle: 'Brave Browser UI',
  brandUrl: 'https://github.com/brave/brave-core'
})

addons.setConfig({
  isFullscreen: false,
  showNav: true,
  showPanel: true,
  panelPosition: 'right',
  theme: braveTheme
})
