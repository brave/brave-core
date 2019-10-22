/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  IMPORT_BROWSER_DATA_REQUESTED = '@@welcome/IMPORT_BROWSER_DATA_REQUESTED',
  GO_TO_TAB_REQUESTED = '@@welcome/GO_TO_TAB_REQUESTED',
  CLOSE_TAB_REQUESTED = '@@welcome/CLOSE_TAB_REQUESTED',
  CHANGE_DEFAULT_SEARCH_PROVIDER = '@@welcome/CHANGE_DEFAULT_SEARCH',
  IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS = '@@welcome/IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS',
  IMPORT_BROWSER_PROFILES_SUCCESS = '@@welcome/IMPORT_BROWSER_PROFILES_SUCCESS',
  IMPORT_BROWSER_THEMES_SUCCESS = '@@welcome/IMPORT_BROWSER_THEMES_SUCCESS',
  SET_BROWSER_THEME = '@@welcome/SET_BROWSER_THEME',
  RECORD_P3A = '@welcome/RECORD_P3A',
  CREATE_WALLET = '@@welcome/CREATE_WALLET',
  ON_WALLET_INITIALIZED = '@@welcome/ON_WALLET_INITIALIZED'
}
