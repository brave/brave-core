/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const enum PublisherStatus {
  NOT_VERIFIED = 0,
  UPHOLD_VERIFIED = 2,
  BITFLYER_VERIFIED = 3,
  GEMINI_VERIFIED = 4
}

// Returns the external wallet provider name for the specified publisher status.
export function publisherStatusToWalletProviderName (status: PublisherStatus) {
  switch (status) {
    case PublisherStatus.UPHOLD_VERIFIED:
      return 'Uphold'
    case PublisherStatus.BITFLYER_VERIFIED:
      return 'bitFlyer'
    case PublisherStatus.GEMINI_VERIFIED:
      return 'Gemini'
    default:
      return ''
  }
}

export function isPublisherVerified (status: PublisherStatus) {
  return status !== PublisherStatus.NOT_VERIFIED
}
