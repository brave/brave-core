// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Modal, Wrapper } from './standard-modal.style'

interface Props {
  children?: React.ReactNode
  modalHeight?: 'standard' | 'full' | 'dynamic'
  modalBackground?: 'background01' | 'background02'
}

export const StandardModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const { children, modalHeight, modalBackground } = props
    return (
      <Wrapper>
        <Modal
          ref={forwardedRef}
          modalHeight={modalHeight}
          modalBackground={modalBackground}
        >
          {children}
        </Modal>
      </Wrapper>
    )
  }
)
