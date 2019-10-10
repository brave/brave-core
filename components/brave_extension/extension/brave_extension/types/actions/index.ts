import { shieldPanelActions } from './shieldsPanelActions'
import { tabActions } from './tabActions'
import { webNavigationActions } from './webNavigationActions'
import { windowActions } from './windowActions'
import { runtimeActions } from './runtimeActions'

export type Actions =
  shieldPanelActions |
  tabActions |
  webNavigationActions |
  windowActions |
  runtimeActions
