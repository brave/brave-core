// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

import { getLocale } from '../../../../../common/locale'
import { WalletRoutes } from '../../../../constants/types'
import { reduceAddress } from '../../../../utils/reduce-address'
import { getAccountTypeDescription } from '../../../../utils/account-utils'
import {
  useGetHiddenAccountsQuery,
  useRemoveHiddenAccountMutation,
} from '../../../../common/slices/api.slice'
import PopupModal from '..'
import { Checkbox } from '../../../shared/checkbox/checkbox'
import { Column, LeoSquaredButton, Row, Text } from '../../../shared/style'
import { CreateAccountIcon } from '../../../shared/create-account-icon/create-account-icon'

const RestoreAccountsList = styled(Column)`
  max-height: 320px;
  overflow: auto;
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 8px;
  padding: 8px;
`

export const RestoreAccountsModal = () => {
  const history = useHistory()
  const { data: hiddenAccounts = [] } = useGetHiddenAccountsQuery()
  const [removeHiddenAccount] = useRemoveHiddenAccountMutation()
  const [selectedAccountKeys, setSelectedAccountKeys] = React.useState<
    Set<string>
  >(new Set())

  const onClose = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
  }, [history])

  const onToggleAccountSelected = React.useCallback((accountUniqueKey: string) => {
    setSelectedAccountKeys((currentKeys) => {
      const nextKeys = new Set(currentKeys)
      if (nextKeys.has(accountUniqueKey)) {
        nextKeys.delete(accountUniqueKey)
      } else {
        nextKeys.add(accountUniqueKey)
      }
      return nextKeys
    })
  }, [])

  const onRestoreSelectedAccounts = React.useCallback(async () => {
    const accountsToRestore = hiddenAccounts.filter((account) =>
      selectedAccountKeys.has(account.accountId.uniqueKey),
    )

    await Promise.all(
      accountsToRestore.map((account) =>
        removeHiddenAccount({ accountId: account.accountId }).unwrap(),
      ),
    )

    onClose()
  }, [hiddenAccounts, onClose, removeHiddenAccount, selectedAccountKeys])

  return (
    <PopupModal
      title={getLocale('braveWalletWelcomeRestoreButton')}
      onClose={onClose}
      width='480px'
    >
      <Column
        fullWidth
        alignItems='flex-start'
        padding='0px 24px 24px'
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
                  <Text
                    textColor='primary'
                    textSize='14px'
                    isBold={true}
                  >
                    {account.name}
                  </Text>
                  <Text
                    textColor='secondary'
                    textSize='12px'
                    isBold={false}
                  >
                    {getAccountTypeDescription(account.accountId)}
                  </Text>
                  <Text
                    textColor='secondary'
                    textSize='12px'
                    isBold={false}
                  >
                    {account.address ? reduceAddress(account.address) : ' '}
                  </Text>
                </Column>
              </Row>
            </Checkbox>
          ))}
        </RestoreAccountsList>
        <Row
          justifyContent='flex-end'
          gap='12px'
          margin='16px 0px 0px 0px'
        >
          <LeoSquaredButton
            kind='plain-faint'
            onClick={onClose}
          >
            {getLocale('braveWalletButtonCancel')}
          </LeoSquaredButton>
          <LeoSquaredButton
            kind='filled'
            isDisabled={selectedAccountKeys.size === 0}
            onClick={onRestoreSelectedAccounts}
          >
            {getLocale('braveWalletWelcomeRestoreButton')}
          </LeoSquaredButton>
        </Row>
      </Column>
    </PopupModal>
  )
}
