// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// types
import { TabNavTypes } from '../../../constants/types'

// options
import { CUSTOM_ASSET_NAV_OPTIONS } from '../../../options/add-custom-asset-nav-options'

// components
import { TopTabNav } from '../index'
import { AddCustomTokenForm } from '../../shared/add-custom-token-form/add-custom-token-form'
import { AddNftForm } from '../../shared/add-custom-token-form/add-nft-form'

// styles
import { AddAssetWrapper } from './add-asset.styles'

interface Props {
  contractAddress: string | undefined
  onHideForm: () => void
}

export const AddAsset = (props: Props) => {
  const {
    contractAddress,
    onHideForm
  } = props
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>(contractAddress || '')
  const [selectedTab, setSelectedTab] = React.useState<TabNavTypes>('token')

  const onSelectTab = React.useCallback((id: TabNavTypes) => {
    // Reset contractAddress when a user switches tabs
    // This will reset the form to avoid the tabs being auto selected based
    // on found token type
    if (tokenContractAddress !== '') setTokenContractAddress('')
    setSelectedTab(id)
  }, [tokenContractAddress])

  const onNftAssetFound = React.useCallback((contractAddress: string) => {
    setTokenContractAddress(contractAddress)
    setSelectedTab('nft')
  }, [])

  const onTokenFound = React.useCallback((contractAddress: string) => {
    setTokenContractAddress(contractAddress)
    setSelectedTab('token')
  }, [])

  const onChangeContractAddress = React.useCallback((contractAddress: string) => {
    setTokenContractAddress(contractAddress)
  }, [])

  return (
    <AddAssetWrapper>
      <TopTabNav
        tabList={CUSTOM_ASSET_NAV_OPTIONS}
        selectedTab={selectedTab}
        onSelectTab={onSelectTab}
      />

      {selectedTab === 'token'
        ? <AddCustomTokenForm
          contractAddress={tokenContractAddress}
          onHideForm={onHideForm}
          onNftAssetFound={onNftAssetFound}
          onChangeContractAddress={onChangeContractAddress}
        />
        : <AddNftForm
          contractAddress={tokenContractAddress}
          onHideForm={onHideForm}
          onTokenFound={onTokenFound}
          onChangeContractAddress={onChangeContractAddress}
        />
      }
    </AddAssetWrapper>
  )
}
