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
  // Typescript doesn't include Temporal or Intl.DurationFormat yet, but both
  // are supported in Chromium. Only the parts we use are declared here.
  // TODO: Remove this once upgrade to TS >= 6.0 is complete
  namespace Temporal {
    interface Duration {}

    class Instant {
      static fromEpochMilliseconds(epochMilliseconds: number): Instant
      until(
        other: Instant,
        options: {
          largestUnit: 'hours'
          smallestUnit: 'minutes'
          roundingMode: 'ceil'
        },
      ): Duration
    }

    namespace Now {
      function instant(): Instant
    }
  }

  namespace Intl {
    class DurationFormat {
      constructor(locales: undefined, options: { style: 'long' })
      format(duration: Temporal.Duration): string
    }
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
  }
}
