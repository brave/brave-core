// Copyright (c) 2023 The Brave Authors. All rights reserved.
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
import { AddNftForm } from '../../../shared/add-custom-token-form/add-nft-form'

// Styles
import { StyledWrapper } from './add-edit-nft-modal.style'

interface Props {
  nftToken?: BraveWallet.BlockchainToken
  onClose: () => void
  onHideForm: () => void
}

export const AddOrEditNftModal = ({ nftToken, onClose, onHideForm }: Props) => {
  const [contractAddress, setContractAddress] = React.useState<string>(
    nftToken?.contractAddress || ''
  )

  return (
    <PopupModal
      title={
        nftToken
          ? getLocale('braveWalletEditNftModalTitle')
          : getLocale('braveWalletAddNftModalTitle')
      }
      onClose={onClose}
      width='584px'
      showDivider={false}
      headerPaddingHorizontal={'32px'}
    >
      <StyledWrapper>
        <AddNftForm
          selectedAsset={nftToken}
          contractAddress={contractAddress}
          onHideForm={onHideForm}
          onChangeContractAddress={setContractAddress}
        />
      </StyledWrapper>
    </PopupModal>
  )
}
