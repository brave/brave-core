// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'

// Utils
import {
  getAccountTypeDescription
} from '../../../../../utils/account-utils'
import { getLocale } from '../../../../../../common/locale'

import {
  useGetAccountInfosRegistryQuery
} from '../../../../../common/slices/api.slice'
import {
  selectAllAccountInfosFromQuery
} from '../../../../../common/slices/entities/account-info.entity'

// Components
import {
  CreateAccountIcon
} from '../../../../shared/create-account-icon/create-account-icon'

// Styled Components
import {
  SelectAllButton,
  Title,
  CheckboxText,
  Description
} from './filter-components.style'
import { Row, Column } from '../../../../shared/style'

interface Props {
  filteredOutAccountAddresses: string[]
  setFilteredOutAccountAddresses: (addresses: string[]) => void
}

export const FilterAccountsSection = (props: Props) => {
  const {
    filteredOutAccountAddresses,
    setFilteredOutAccountAddresses
  } = props

  const { data: accountsList } =
    useGetAccountInfosRegistryQuery(undefined,
      {
        selectFromResult: (res) => ({
          isLoading: res.isLoading,
          data: selectAllAccountInfosFromQuery(res)
        })
      })

  // Methods
  const isAccountFilteredOut = React.useCallback(
    (address: string) => {
      return filteredOutAccountAddresses.includes(address)
    }, [filteredOutAccountAddresses])

  const onCheckAccount = React.useCallback((address: string) => {
    if (isAccountFilteredOut(address)) {
      setFilteredOutAccountAddresses(
        filteredOutAccountAddresses
          .filter((addressKey) => addressKey !== address))
      return
    }
    setFilteredOutAccountAddresses(
      [...filteredOutAccountAddresses, address]
    )
  }, [
    filteredOutAccountAddresses,
    isAccountFilteredOut,
    setFilteredOutAccountAddresses
  ])

  const onSelectOrDeselectAllAccounts = React.useCallback(
    () => {
      if (filteredOutAccountAddresses.length > 0) {
        setFilteredOutAccountAddresses([])
        return
      }
      setFilteredOutAccountAddresses(
        accountsList.map((account) => account.address)
      )
    }, [
    accountsList,
    filteredOutAccountAddresses,
    setFilteredOutAccountAddresses
  ])

  return (
    <>
      <Row
        marginBottom={8}
        justifyContent='space-between'
      >
        <Title
          textSize='16px'
          isBold={true}
        >
          {getLocale('braveWalletSelectAccounts')}
        </Title>
        <SelectAllButton
          onClick={onSelectOrDeselectAllAccounts}
        >
          {
            filteredOutAccountAddresses.length > 0
              ? getLocale('braveWalletSelectAll')
              : getLocale('braveWalletDeselectAll')
          }
        </SelectAllButton>
      </Row>
      <Column
        justifyContent='flex-start'
        alignItems='flex-start'
        fullWidth={true}
      >
        {accountsList.map((account) =>
          <Row
            width='unset'
            justifyContent='flex-start'
            marginBottom={16}
            key={account.accountId.uniqueKey}
          >
            <Checkbox
              checked={
                !isAccountFilteredOut(account.address)
              }
              onChanged={
                () => onCheckAccount(account.address)
              }
            >
              <CreateAccountIcon
                size='medium'
                account={account}
              />
              <Column
                alignItems='flex-start'
              >
                <CheckboxText
                  textSize='14px'
                  isBold={false}
                >
                  {account.name}
                </CheckboxText>
                <Description
                  textSize='12px'
                  isBold={false}
                >
                  {getAccountTypeDescription(account.accountId.coin)}
                </Description>
              </Column>
            </Checkbox>
          </Row>
        )}
      </Column>
    </>

  )
}
