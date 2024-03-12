// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import { PopupModal } from '../../popup-modals/index'
import {
  AddCustomTokenForm //
} from '../../../shared/add-custom-token-form/add-custom-token-form'

// Styles
import { StyledWrapper } from './edit_token_modal.style'

interface Props {
  token: BraveWallet.BlockchainToken
  onClose: () => void
}

export const EditTokenModal = ({ token, onClose }: Props) => {
  return (
    <PopupModal
      title={getLocale('braveWalletEditToken')}
      onClose={onClose}
      width='584px'
      showDivider={true}
    >
      <StyledWrapper>
        <AddCustomTokenForm
          selectedAsset={token}
          onHideForm={onClose}
        />
      </StyledWrapper>
    </PopupModal>
  )
}
