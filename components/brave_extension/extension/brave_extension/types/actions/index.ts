import { shieldPanelActions } from './shieldsPanelActions'
import { tabActions } from './tabActions'
import { webNavigationActions } from './webNavigationActions'
import { windowActions } from './windowActions'
import { cosmeticFilterActions } from './cosmeticFilterActions'
import { runtimeActions } from './runtimeActions'

export type Actions =
  shieldPanelActions |
  tabActions |
  webNavigationActions |
  windowActions |
  cosmeticFilterActions |
  runtimeActions
