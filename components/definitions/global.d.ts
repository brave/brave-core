// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

export {}

type RequestIdleCallbackHandle = any
type RequestIdleCallbackOptions = {
  timeout: number
}
type RequestIdleCallbackDeadline = {
  readonly didTimeout: boolean;
  timeRemaining: (() => number)
}

declare global {
  type StyleAction = {
    type: 'style'
    arg: string
  }
  type RemoveAction = {
    type: 'remove'
  }
  type RemoveAttrAction = {
    type: 'remove-attr'
    arg: string
  }
  type RemoveClassAction = {
    type: 'remove-class'
    arg: string
  }

  type CosmeticFilterAction = StyleAction | RemoveAction | RemoveAttrAction | RemoveClassAction;

  interface ProceduralOperator {
    type: OperatorType
    arg: string
  }
  type ProceduralSelector = ProceduralOperator[];
  type ProceduralActionFilter = {
    selector: ProceduralSelector
    action?: CosmeticFilterAction
  }
  interface Window {
    // Typescript doesn't include requestIdleCallback as it's non-standard.
    // Since it's supported in Chromium, we can include it here.
    requestIdleCallback: ((
      callback: ((deadline: RequestIdleCallbackDeadline) => void),
      opts?: RequestIdleCallbackOptions
    ) => RequestIdleCallbackHandle)
    cancelIdleCallback: ((handle: RequestIdleCallbackHandle) => void)
    alreadyInserted: boolean
    web3: any
    content_cosmetic: {
      cosmeticStyleSheet: CSSStyleSheet
      allSelectorsToRules: Map<string, number>
      observingHasStarted: boolean
      hide1pContent: boolean
      generichide: boolean
      firstRunQueue: Set<string>
      secondRunQueue: Set<string>
      finalRunQueue: Set<string>
      allQueues: Set<string>[]
      numQueues: any
      alreadyUnhiddenSelectors: Set<string>
      alreadyKnownFirstPartySubtrees: WeakSet
      _hasDelayOcurred: boolean
      _startCheckingId: number | undefined
      firstSelectorsPollingDelayMs: number | undefined
      switchToSelectorsPollingThreshold : number | undefined
      fetchNewClassIdRulesThrottlingMs : number | undefined
      tryScheduleQueuePump: (() => void)
      proceduralActionFilters?: ProceduralActionFilter[]
      hasProceduralActions: boolean,
      setTheme: (bgcolor: number) => void
    }
  }
}
