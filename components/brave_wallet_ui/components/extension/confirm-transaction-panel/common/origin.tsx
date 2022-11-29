// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { FavIcon } from './style'
import BraveIcon from '../../../../assets/svg-icons/brave-icon.svg'
import { URLText } from '../../shared-panel-styles'
import CreateSiteOrigin from '../../../shared/create-site-origin'

// Types
import { SerializableOriginInfo } from '../../../../constants/types'

interface Props {
  originInfo: SerializableOriginInfo
}

export function Origin (props: Props) {
  const { originInfo } = props
  return (
    <>
      <FavIcon
        src={
          originInfo.originSpec.startsWith('chrome://wallet')
            ? BraveIcon
            : `chrome://favicon/size/64@1x/${originInfo.originSpec}`
        }
      />
      <URLText>
        <CreateSiteOrigin originSpec={originInfo.originSpec} eTldPlusOne={originInfo.eTldPlusOne} />
      </URLText>
    </>
  )
}
