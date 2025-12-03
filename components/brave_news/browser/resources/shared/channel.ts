// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getString } from '../strings'

const getChannelStringId = (channelName: string) => 'BRAVE_NEWS_CHANNEL_'
  + channelName.toUpperCase()
    .split(' ')
    .join('_')

export const getTranslatedChannelName = (channelName: string) => {
  if (!channelName) return ''
  try {
    return getString(getChannelStringId(channelName) as any)
  } catch (err) {
    console.error(`Couldn't find translation for channel '${channelName}'`)
    return channelName
  }
}
