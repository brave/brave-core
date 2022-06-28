// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// hooks
import { useCopy } from '../../../common/hooks'

// utils
import { getLocale } from '../../../../common/locale'

// components
import { Tooltip } from '../index'

// styled components
import { StyledWrapper } from './style'

interface Props {
  children: React.ReactNode
  tooltipText?: string
  actionText?: string
  text?: string
}

export const CopyTooltip = ({ children, tooltipText, actionText, text }: Props) => {
  const { copied, copyText: copyToClipboard } = useCopy()

  const handleClick = React.useCallback(async () => {
    if (text) {
      await copyToClipboard(text)
    }
  }, [text, copyToClipboard])

  return (
    <StyledWrapper onClick={handleClick}>
      <Tooltip
        text={tooltipText || getLocale('braveWalletToolTipCopyToClipboard')}
        actionText={actionText || getLocale('braveWalletToolTipCopiedToClipboard')}
        isActionVisible={copied}
      >
        {children}
      </Tooltip>
    </StyledWrapper>
  )
}

export default CopyTooltip
