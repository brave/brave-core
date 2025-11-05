/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import { getPlaylistAPI } from '../api/api'
import { getLocalizedString } from '../utils/l10n'

export function AddMediaFromPageButton () {
  return (
    <Button
      kind='outline'
      onClick={() => {
        getPlaylistAPI().showAddMediaToPlaylistUI()
      }}
    >
      <div slot='icon-before'>
        <Icon name='product-playlist-add' />
      </div>
      <div>{getLocalizedString('bravePlaylistAddMediaFromPage')}</div>
    </Button>
  )
}
