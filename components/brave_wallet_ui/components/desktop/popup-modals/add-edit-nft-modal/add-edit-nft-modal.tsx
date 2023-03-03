// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { BraveWallet } from '../../../../constants/types'
import { PopupModal } from '../..'
import { AddNftForm } from '../../../shared/add-custom-token-form/add-nft-form'
import { StyledWrapper } from './add-edit-nft-modal.style'

interface Props {
  nftToken?: BraveWallet.BlockchainToken
  onClose: () => void
  onHideForm: () => void
  onTokenFound?: (contractAddress: string) => void
}

export const AddOrEditNftModal = ({ nftToken, onClose, onHideForm, onTokenFound }: Props) => {
  const [contractAddress, setContractAddress] = React.useState<string>(nftToken?.contractAddress || '')

  return (
    <PopupModal
      title={nftToken ? 'Import NFT' : 'Edit NFT'}
      onClose={onClose}
      width='584px'
      showDivider={true}
    >
      <StyledWrapper>
        <AddNftForm
          selectedAsset={nftToken}
          contractAddress={contractAddress}
          onHideForm={onHideForm}
          onTokenFound={onTokenFound}
          onChangeContractAddress={setContractAddress}
          />
      </StyledWrapper>
    </PopupModal>
  )
}
