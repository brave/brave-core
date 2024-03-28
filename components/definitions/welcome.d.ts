//  Copyright (c) 2018 The Brave Authors. All rights reserved.
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this file,
//  You can obtain one at https://mozilla.org/MPL/2.0/.

declare namespace Welcome {
  export interface ApplicationState {
    welcomeData: State | undefined
  }

  export interface BrowserProfile {
    autofillFormData: boolean,
    cookies: boolean,
    favorites: boolean,
    history: boolean,
    index: number,
    ledger: boolean,
    name: string,
    passwords: boolean,
    search: boolean,
    stats: boolean,
    windows: boolean
  }

  export interface SearchEngineEntry {
    canBeDefault: boolean,
    canBeEdited: boolean,
    canBeRemoved: boolean,
    default: boolean,
    displayName: string,
    iconURL: string,
    id: number,
    isOmniboxExtension: boolean,
    keyword: string,
    modelIndex: number,
    name: string,
    url: string,
    urlLocked: boolean
  }

  export interface SearchEngineListResponse {
    defaults: Array<SearchEngineEntry>
    extensions: Array<any>
    others: Array<any>
  }

  export interface State {
    searchProviders: Array<SearchEngineEntry>
    browserProfiles: Array<BrowserProfile>,
    showSearchCard: boolean,
    showRewardsCard: boolean
  }
}
