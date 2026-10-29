// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Button from '@brave/leo/react/button'

// Constants
import { WalletRoutes } from '$wallet/constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import { reduceAddress } from '$wallet/utils/reduce-address'
import { getAccountTypeDescription } from '$wallet/utils/account-utils'
import {
  useGetHiddenAccountsQuery,
  useRemoveHiddenAccountsMutation,
} from '$wallet/common/slices/api.slice'

// Components
import PopupModal from '..'
import { Checkbox } from '$wallet/components/shared/checkbox/checkbox'
import { Column, Row } from '$wallet/components/shared/style'
import { CreateAccountIcon } from '$wallet/components/shared/create-account-icon/create-account-icon'

// Styled Components
import {
  AccountMetaText,
  AccountNameText,
  ContentColumn,
  FooterRow,
  UnhideAccountsList,
  CancelButton,
} from './unhide_accounts_modal.style'

export const UnhideAccountsModal = () => {
  const history = useHistory()

  // Queries
  const { data: hiddenAccounts = [] } = useGetHiddenAccountsQuery()
  const [removeHiddenAccounts] = useRemoveHiddenAccountsMutation()

  // State
  const [selectedAccountKeys, setSelectedAccountKeys] = React.useState<
    Set<string>
  >(new Set())

  // Methods
  const onClose = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
  }, [history])

  const onToggleAccountSelected = React.useCallback(
    (accountUniqueKey: string) => {
      setSelectedAccountKeys((currentKeys) => {
        const nextKeys = new Set(currentKeys)
        if (nextKeys.has(accountUniqueKey)) {
          nextKeys.delete(accountUniqueKey)
        } else {
          nextKeys.add(accountUniqueKey)
        }
        return nextKeys
      })
    },
    [],
  )

  const onUnhideSelectedAccounts = React.useCallback(async () => {
    const accountsToUnhide = hiddenAccounts.filter((account) =>
      selectedAccountKeys.has(account.accountId.uniqueKey),
    )
    const accountIds = accountsToUnhide.map((account) => account.accountId)

    await removeHiddenAccounts(accountIds).unwrap()

    onClose()
  }, [hiddenAccounts, onClose, removeHiddenAccounts, selectedAccountKeys])

  return (
    <PopupModal
      title={getLocale(S.BRAVE_WALLET_UNHIDE_ACCOUNT)}
      onClose={onClose}
      width='480px'
    >
      <ContentColumn
        fullWidth
        alignItems='flex-start'
      >
        <UnhideAccountsList
          fullWidth
          alignItems='flex-start'
          justifyContent='flex-start'
        >
          {hiddenAccounts.map((account) => (
            <Checkbox
              key={account.accountId.uniqueKey}
              isChecked={selectedAccountKeys.has(account.accountId.uniqueKey)}
              onChange={() =>
                onToggleAccountSelected(account.accountId.uniqueKey)
              }
              alignItems='center'
            >
              <Row
                width='unset'
                alignItems='center'
              >
                <CreateAccountIcon
                  account={account}
                  size='big'
                  marginRight={12}
                />
                <Column
                  width='unset'
                  alignItems='flex-start'
                >
                  <AccountNameText>{account.name}</AccountNameText>
                  <AccountMetaText>
                    {getAccountTypeDescription(account.accountId)}
                  </AccountMetaText>
                  <AccountMetaText>
                    {account.address ? reduceAddress(account.address) : ' '}
                  </AccountMetaText>
                </Column>
              </Row>
            </Checkbox>
          ))}
        </UnhideAccountsList>
        <FooterRow>
          <CancelButton
            kind='plain-faint'
            onClick={onClose}
          >
            {getLocale(S.BRAVE_WALLET_BUTTON_CANCEL)}
          </CancelButton>
          <Button
            kind='filled'
            isDisabled={selectedAccountKeys.size === 0}
            onClick={onUnhideSelectedAccounts}
          >
            {getLocale(S.BRAVE_WALLET_UNHIDE)}
          </Button>
        </FooterRow>
      </ContentColumn>
    </PopupModal>
  )
}
