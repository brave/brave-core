/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Flags an ad/segment/creative set elsewhere in the UI that the user has
// already reacted to (see reactions.tsx), so the reaction isn't only
// discoverable by cross-referencing IDs by hand.
export function ReactionIcon({ type }: {
  type: 'liked' | 'disliked' | 'saved' | 'inappropriate'
}) {
  switch (type) {
    case 'liked':
      return <Icon name='thumb-up' className='icon-success' title='Liked' />
    case 'disliked':
      return <Icon name='thumb-down' className='icon-error' title='Disliked' />
    case 'saved':
      return <Icon name='star-outline' className='icon-success' title='Saved' />
    case 'inappropriate':
      return <Icon name='flag' className='icon-error' title='Marked as inappropriate' />
  }
}
