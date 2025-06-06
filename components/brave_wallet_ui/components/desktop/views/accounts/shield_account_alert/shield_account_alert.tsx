// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Safe Selectors
import {
  useSafeUISelector, //
} from '../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../common/selectors'

// Components
import {
  ShieldZCashAccountModal, //
} from '../../../popup-modals/shield_zcash_account/shield_zcash_account'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import { Wrapper, Alert } from './shield_account_alert.style'
import { Row, Column, Text } from '../../../../shared/style'

export interface Props {
  account: BraveWallet.AccountInfo
}

export function ShieldAccountAlert(props: Props) {
  const { account } = props

  // State
  const [showShieldAccountModal, setShowShieldAccountModal] =
    React.useState<boolean>(false)

  // Safe Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isAndroid = useSafeUISelector(UISelectors.isAndroid)
  const isPanelOrAndroid = isPanel || isAndroid

  return (
    <>
      <Wrapper>
        <Alert type='info'>
          <Row justifyContent='space-between'>
            <Column alignItems='flex-start'>
              <Text
                textColor='info'
                isBold={true}
                textSize='16px'
              >
                {getLocale('braveWalletShieldAccount')}
              </Text>
              {getLocale('braveWalletShieldAccountAlertDescription')}
            </Column>
            {!isPanelOrAndroid && (
              <div>
                <Button
                  onClick={() => {
                    setShowShieldAccountModal(true)
                  }}
                >
                  <Icon
                    name='shield-done'
                    slot='icon-before'
                  />
                  {getLocale('braveWalletShieldAccount')}
                </Button>
              </div>
            )}
          </Row>
          {isPanelOrAndroid && (
            <div slot='actions'>
              <Button
                size='small'
                onClick={() => {
                  setShowShieldAccountModal(true)
                }}
              >
                <Icon
                  name='shield-done'
                  slot='icon-before'
                />
                {getLocale('braveWalletShieldAccount')}
              </Button>
            </div>
          )}
        </Alert>
      </Wrapper>
      {showShieldAccountModal && (
        <ShieldZCashAccountModal
          account={account}
          onClose={() => {
            setShowShieldAccountModal(false)
          }}
        />
      )}
    </>
  )
}

export default ShieldAccountAlert
