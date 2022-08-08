// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { Publishers } from '../../../../api/brave_news'

export interface Props {
  publishers?: Publishers
  setPublisherPref: (publisherId: string, enabled: boolean) => any
  onDisplay: () => any
  onClearPrefs: () => any
  showToday: boolean
  toggleShowToday: () => any
  showBraveNewsButton: boolean
  featureFlagBraveNewsSubscribeButtonEnabled: boolean
  toggleShowBraveNewsButton: () => any
}

export default function BraveTodayPrefs (props: Props) {
  return null
}
