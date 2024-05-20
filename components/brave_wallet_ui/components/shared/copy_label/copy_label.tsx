// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'

// utils
import {
  useCopyToClipboard //
} from '../../../common/hooks/use-copy-to-clipboard'

// styles
import { LabelText } from './copy_label.styles'

export const CopyLabel = ({
  children,
  textToCopy
}: {
  textToCopy: string
  children: React.ReactNode
}) => {
  // custom hooks
  const { copyToClipboard, isCopied } = useCopyToClipboard()

  // render
  return (
    <Label color={isCopied ? 'green' : 'gray'}>
      <div
        slot='icon-after'
        style={{ cursor: isCopied ? 'default' : 'pointer' }}
        onClick={() => {
          if (!isCopied) {
            copyToClipboard(textToCopy)
          }
        }}
      >
        <Icon name={isCopied ? 'check-normal' : 'copy'} />
      </div>
      <LabelText>{children}</LabelText>
    </Label>
  )
}

export default CopyLabel
