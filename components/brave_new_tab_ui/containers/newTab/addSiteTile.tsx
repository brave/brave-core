// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../common/locale'
import { AddSiteTile, AddSiteTileImage, TileTitle } from '../../components/default'
// Icons
import AddSiteTileIcon from '../../components/default/gridSites/assets/add-site-tile'

interface Props {
  showEditTopSite: () => void
  isDragging: boolean
}

export default function AddSite ({ showEditTopSite, isDragging }: Props) {
  return <AddSiteTile onClick={showEditTopSite} isDragging={isDragging}>
      <AddSiteTileImage>
        <AddSiteTileIcon />
      </AddSiteTileImage>
      <TileTitle>
        {getLocale('addTopSiteDialogTitle')}
      </TileTitle>
    </AddSiteTile>
}
