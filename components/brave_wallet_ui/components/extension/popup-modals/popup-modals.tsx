// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  Header,
  Title,
  CloseButton,
  Modal
} from './popup-modals.style'

interface Props {
  children?: React.ReactNode
  onClose: () => void
  title: string
}

const ESC_KEY = 'Escape'

export const PopupModal = (props: Props) => {
  const { title, onClose, children } = props

  const handleKeyDown = React.useCallback((event: KeyboardEvent) => {
    if (event.key === ESC_KEY) {
      onClose()
    }
  }, [onClose])

  React.useEffect(() => {
    document.addEventListener('keydown', handleKeyDown)

    return () => {
      document.removeEventListener('keydown', handleKeyDown)
    }
  }, [handleKeyDown])

  return (
    <StyledWrapper>
      <Modal>
        <Header>
          <Title>{title}</Title>
          <CloseButton onClick={onClose} />
        </Header>
        {children}
      </Modal>
    </StyledWrapper>
  )
}

export default PopupModal
