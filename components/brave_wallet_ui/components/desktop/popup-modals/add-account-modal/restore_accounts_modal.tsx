// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Button from '@brave/leo/react/button'

import { getLocale } from '../../../../../common/locale'
import { WalletRoutes } from '../../../../constants/types'
import { reduceAddress } from '../../../../utils/reduce-address'
import { getAccountTypeDescription } from '../../../../utils/account-utils'
import {
  useGetHiddenAccountsQuery,
  useRemoveHiddenAccountsMutation,
} from '../../../../common/slices/api.slice'
import PopupModal from '..'
import { Checkbox } from '../../../shared/checkbox/checkbox'
import { Column, Row } from '../../../shared/style'
import { CreateAccountIcon } from '../../../shared/create-account-icon/create-account-icon'
import {
  AccountMetaText,
  AccountNameText,
  ContentColumn,
  FooterRow,
  RestoreAccountsList,
} from './restore_accounts_modal.style'

export const RestoreAccountsModal = () => {
  const history = useHistory()
  const { data: hiddenAccounts = [] } = useGetHiddenAccountsQuery()
  const [removeHiddenAccounts] = useRemoveHiddenAccountsMutation()
  const [selectedAccountKeys, setSelectedAccountKeys] = React.useState<
    Set<string>
  >(new Set())

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

  const onRestoreSelectedAccounts = React.useCallback(async () => {
    const accountsToRestore = hiddenAccounts.filter((account) =>
      selectedAccountKeys.has(account.accountId.uniqueKey),
    )
    const accountIds = accountsToRestore.map((account) => account.accountId)

    await removeHiddenAccounts(accountIds).unwrap()

    onClose()
  }, [hiddenAccounts, onClose, removeHiddenAccounts, selectedAccountKeys])

  return (
    <PopupModal
      title={getLocale('braveWalletAccountsRestore')}
      onClose={onClose}
      width='480px'
    >
      <ContentColumn
        fullWidth
        alignItems='flex-start'
      >
        <RestoreAccountsList
          fullWidth
          alignItems='flex-start'
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
        </RestoreAccountsList>
        <FooterRow>
          <Button
            kind='plain-faint'
            onClick={onClose}
          >
            {getLocale('braveWalletButtonCancel')}
          </Button>
          <Button
            kind='filled'
            isDisabled={selectedAccountKeys.size === 0}
            onClick={onRestoreSelectedAccounts}
          >
            {getLocale('braveWalletAccountsRestore')}
          </Button>
        </FooterRow>
      </ContentColumn>
    </PopupModal>
  )
}
