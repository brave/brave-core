// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

declare namespace AdBlock {
  export interface ApplicationState {
    adblockData: State | undefined
  }

  export interface State {
    settings: {
      customFilters: string
      regionalLists: FilterList[]
      listSubscriptions: SubscriptionInfo[]
    }
  }

  export interface FilterList {
    uuid: string
    url: string
    title: string
    supportUrl: string
    componentId: string
    enabled: boolean
  }

  export interface SubscriptionInfo {
    subscription_url: string
    last_update_attempt: number
    last_successful_update_attempt: number
    enabled: boolean
    title?: string
    homepage?: string
  }
}
