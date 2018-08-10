import { shieldPanelActions } from './shieldsPanelActions'
import { tabActions } from './tabActions'
import { webNavigationActions } from './webNavigationActions'
import { windowActions } from './windowActions'
import { cosmeticFilterActions } from './cosmeticFilterActions'

export type Actions =
  shieldPanelActions |
  tabActions |
  webNavigationActions |
  windowActions |
  cosmeticFilterActions
