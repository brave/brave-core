// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '../../../common/locale'
import { AddSiteTile, TileImageContainer, TileTitle } from '../../components/default'

interface Props {
  showEditTopSite: () => void
  isDragging: boolean
}

export default function AddSite ({ showEditTopSite, isDragging }: Props) {
  return <AddSiteTile onClick={showEditTopSite} isDragging={isDragging}>
      <TileImageContainer>
        <Icon name='plus-add' />
      </TileImageContainer>
      <TileTitle>
        {getLocale('addTopSiteDialogTitle')}
      </TileTitle>
    </AddSiteTile>
}
