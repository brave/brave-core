// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Options
import {
  SwapAndSendOptions //
} from '../../../../../../options/swap-and-send-options'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Components
import {
  StandardCheckbox //
} from '../../form-controls/standard-checkbox/standard-checkbox'
import {
  StandardRadio //
} from '../../form-controls/standard-radio/standard-radio'
import {
  StandardSwitch //
} from '../../form-controls/standard-switch/standard-switch'
import { StandardInput } from '../../inputs/standard-input/standard-input'
import { AccountSelector } from '../account-selector/account-selector'

// Styled Components
import { Flash } from './swap-and-send.style'
import {
  Column,
  Row,
  Text,
  VerticalSpacer,
  HorizontalSpacer
} from '../../shared-swap.styles'

interface Props {
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  selectedSwapAndSendOption: string
  toAnotherAddress: string
  selectedSwapSendAccount: BraveWallet.AccountInfo | undefined
  userConfirmedAddress: boolean
  swapAndSendSelected: boolean
  onChangeSwapAndSendSelected: (value: boolean) => void
  onCheckUserConfirmedAddress: (id: string, checked: boolean) => void
  handleOnSetToAnotherAddress: (value: string) => void
  onSelectSwapAndSendOption: (value: string) => void
  onSelectSwapSendAccount: (
    account: BraveWallet.AccountInfo | undefined
  ) => void
}

export const SwapAndSend = (props: Props) => {
  const {
    selectedNetwork,
    selectedSwapAndSendOption,
    toAnotherAddress,
    selectedSwapSendAccount,
    userConfirmedAddress,
    swapAndSendSelected,
    onChangeSwapAndSendSelected,
    onCheckUserConfirmedAddress,
    handleOnSetToAnotherAddress,
    onSelectSwapAndSendOption,
    onSelectSwapSendAccount
  } = props

  // State
  const [showAccountSelector, setShowAccountSelector] =
    React.useState<boolean>(false)

  // Methods
  const handleOnSelectSwapAndSendOption = React.useCallback(
    (value: string) => {
      if (value === 'to-address') {
        onSelectSwapSendAccount(undefined)
        setShowAccountSelector(false)
      }
      onSelectSwapAndSendOption(value)
    },
    [onSelectSwapAndSendOption, onSelectSwapSendAccount]
  )

  return (
    <Column
      columnHeight='dynamic'
      columnWidth='full'
    >
      <VerticalSpacer size={16} />
      <Row
        rowWidth='full'
        marginBottom={16}
        horizontalPadding={16}
      >
        <Row>
          <Text textSize='14px'>{getLocale('braveSwapSwapAndSend')}</Text>
          <Flash
            name='flash'
            size={16}
          />
          <Text
            isBold={false}
            textSize='14px'
            textColor='text03'
          >
            {getLocale('braveSwapNoExtraFees')}
          </Text>
        </Row>
        <StandardSwitch
          isChecked={swapAndSendSelected}
          onSetIsChecked={onChangeSwapAndSendSelected}
        />
      </Row>
      {swapAndSendSelected && (
        <Column
          columnHeight='dynamic'
          columnWidth='full'
          horizontalPadding={16}
        >
          {SwapAndSendOptions.map((option) => (
            <Column
              columnHeight='dynamic'
              columnWidth='full'
              horizontalAlign='flex-start'
              key={option.name}
            >
              <StandardRadio
                id={option.name}
                label={getLocale(option.label)}
                isChecked={option.name === selectedSwapAndSendOption}
                onSetIsChecked={handleOnSelectSwapAndSendOption}
                key={option.name}
              />
              <VerticalSpacer size={10} />
              {option.name === 'to-account' && (
                <>
                  <Row
                    rowWidth='full'
                    horizontalAlign='flex-start'
                  >
                    <HorizontalSpacer size={32} />
                    <AccountSelector
                      onSelectAccount={onSelectSwapSendAccount}
                      selectedAccount={selectedSwapSendAccount}
                      selectedNetwork={selectedNetwork}
                      disabled={selectedSwapAndSendOption === 'to-address'}
                      showAccountSelector={showAccountSelector}
                      setShowAccountSelector={setShowAccountSelector}
                    />
                  </Row>
                  <VerticalSpacer size={16} />
                </>
              )}

              {option.name === 'to-address' && (
                <>
                  <Row rowWidth='full'>
                    <HorizontalSpacer size={32} />
                    <StandardInput
                      placeholder={getLocale(
                        'braveSwapAddressInputePlaceholder'
                      )}
                      onChange={handleOnSetToAnotherAddress}
                      value={toAnotherAddress}
                      disabled={selectedSwapAndSendOption !== 'to-address'}
                    />
                  </Row>
                  {selectedSwapAndSendOption === 'to-address' &&
                    toAnotherAddress !== '' && (
                      <>
                        <VerticalSpacer size={16} />
                        <Row
                          rowWidth='full'
                          horizontalAlign='flex-start'
                        >
                          <HorizontalSpacer size={32} />
                          <StandardCheckbox
                            id='confirm-address'
                            label={getLocale('braveSwapConfirmAddress')}
                            isChecked={userConfirmedAddress}
                            onChange={onCheckUserConfirmedAddress}
                            key='confirm-address'
                          />
                        </Row>
                      </>
                    )}
                </>
              )}
            </Column>
          ))}
        </Column>
      )}
    </Column>
  )
}
