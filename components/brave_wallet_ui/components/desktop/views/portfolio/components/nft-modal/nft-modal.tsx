// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// styles
import {
  StyledWrapper,
  Modal,
  modalSize
} from './nft-modal.styles'
import { NftIcon } from '../../../../../shared/nft-icon/nft-icon'

interface Props {
  nftImageUrl: string
  onClose: () => void
}

const ESC_KEY = 'Escape'

export const NftModal = (props: Props) => {
  const { nftImageUrl, onClose } = props

  const onClickModal = React.useCallback((event: React.MouseEvent<HTMLDivElement>) => {
    event.preventDefault()
  }, [])

  const handleKeyDown = React.useCallback((event: KeyboardEvent) => {
    if (event.key === ESC_KEY) {
      onClose()
    }
  }, [onClose])

  // effects
  React.useEffect(() => {
    document.addEventListener('keydown', handleKeyDown)

    return () => {
      document.removeEventListener('keydown', handleKeyDown)
    }
  }, [handleKeyDown])

  return (
    <StyledWrapper
      onClick={onClose}
    >
      <Modal
        width={modalSize}
        height={modalSize}
        onClick={onClickModal}
      >
        <NftIcon
          iconStyles={{ borderRadius: '10px' }}
          icon={nftImageUrl}
          responsive={true}
        />
      </Modal>
    </StyledWrapper>
  )
}
