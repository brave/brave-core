// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { CloseIcon, Column, Row } from '../../shared/style'
import {
  FullScreenPanelPopupWrapper,
  IconButton
} from './full_panel_popup.style'

interface Props {
  kind?: 'danger'
  onClose?: () => void
}

export const FullPanelPopup: React.FC<Props> = ({
  children,
  kind,
  onClose
}) => {
  return (
    <FullScreenPanelPopupWrapper kind={kind}>
      <Column
        fullHeight
        fullWidth
        justifyContent='flex-start'
      >
        {onClose && (
          <Row
            justifyContent='flex-end'
            alignItems='center'
            padding={'16px'}
          >
            <Column width='20px'>
              <IconButton
                kind='plain'
                onClick={onClose}
              >
                <CloseIcon />
              </IconButton>
            </Column>
          </Row>
        )}
        {children}
      </Column>
    </FullScreenPanelPopupWrapper>
  )
}

export default FullPanelPopup
