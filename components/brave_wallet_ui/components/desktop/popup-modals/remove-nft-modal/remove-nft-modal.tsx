// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'

// selectors
import { useSafeUISelector } from '../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../common/selectors'

// components
import { PopupModal } from '../index'

// styles
import {
  ButtonRow,
  CancelButton,
  ConfirmButton,
  Description,
  Header,
  StyledWrapper
} from './remove-nft-modal.styles'

interface Props {
  onConfirm: () => void
  onCancel: () => void
}

export const RemoveNftModal = ({ onConfirm, onCancel }: Props) => {
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  return (
    <PopupModal
      title=''
      width='456px'
      hideHeader={isPanel}
      onClose={onCancel}
    >
      <StyledWrapper>
        <Header>{getLocale('braveWalletRemoveNftModalHeader')}</Header>
        <Description>
          {getLocale('braveWalletRemoveNftModalDescription')}
        </Description>
        <ButtonRow>
          <CancelButton onClick={onCancel}>
            {getLocale('braveWalletRemoveNftModalCancel')}
          </CancelButton>
          <ConfirmButton onClick={onConfirm}>
            {getLocale('braveWalletRemoveNftModalConfirm')}
          </ConfirmButton>
        </ButtonRow>
      </StyledWrapper>
    </PopupModal>
  )
}
