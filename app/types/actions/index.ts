import { newTabPageActions } from './newTabPageActions'
import { shieldPanelActions } from './shieldsPanelActions'
import { tabActions } from './tabActions'
import { webNavigationActions } from './webNavigationActions'
import { windowActions } from './windowActions'

export type Actions =
  shieldPanelActions |
  newTabPageActions |
  tabActions |
  webNavigationActions |
  windowActions
