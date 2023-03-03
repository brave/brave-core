// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// hooks
import { useCopyToClipboard } from '../../../common/hooks/use-copy-to-clipboard'

// utils
import { getLocale } from '../../../../common/locale'

// components
import { Tooltip, ToolTipProps } from '../tooltip'

// styled components
import { StyledWrapper } from './style'

type Props = {
  children: React.ReactNode
  tooltipText?: string
  actionText?: string
  text?: string
} & ToolTipProps

export const CopyTooltip = ({
  children,
  tooltipText,
  actionText,
  text,
  ...tipProps
}: Props) => {
  const { isCopied, copyToClipboard } = useCopyToClipboard(1500)

  const handleClick = React.useCallback(async () => {
    if (text) {
      await copyToClipboard(text)
    }
  }, [text, copyToClipboard])

  return (
    <StyledWrapper onClick={handleClick}>
      <Tooltip
        text={tooltipText || getLocale('braveWalletToolTipCopyToClipboard')}
        actionText={
          actionText || getLocale('braveWalletToolTipCopiedToClipboard')
        }
        isActionVisible={isCopied}
        {...tipProps}
      >
        {children}
      </Tooltip>
    </StyledWrapper>
  )
}

export default CopyTooltip
