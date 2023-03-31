// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  HeaderTitle,
  HeaderWrapper,
  ActionIcon,
  ExpandIcon
} from './style'

// components
import { WalletMorePopup } from '../../desktop'

export interface Props {
  onExpand: () => void
  onClickMore: () => void
  onClickViewOnBlockExplorer?: () => void
  showMore: boolean
}

export const ConnectedHeader = (props: Props) => {
  const {
    onClickMore,
    onExpand,
    onClickViewOnBlockExplorer,
    showMore
  } = props

  // render
  return (
    <HeaderWrapper>
      <ExpandIcon onClick={onExpand} />
      <HeaderTitle>{getLocale('braveWalletPanelTitle')}</HeaderTitle>
      <ActionIcon onClick={onClickMore} />
      {showMore &&
        <WalletMorePopup
          onClickViewOnBlockExplorer={onClickViewOnBlockExplorer}
        />
      }
    </HeaderWrapper>
  )
}

export default ConnectedHeader
