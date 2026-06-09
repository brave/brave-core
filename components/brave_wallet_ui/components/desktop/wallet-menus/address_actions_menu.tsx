// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { showAlert } from '@brave/leo/react/alertCenter'
import Icon from '@brave/leo/react/icon'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'

// Hooks
import {
  useOnClickOutside, //
} from '../../../common/hooks/useOnClickOutside'

// Components
import {
  DepositModal, //
} from '../popup-modals/account-settings-modal/account-settings-modal'
import {
  ViewOnBlockExplorerModal, //
} from '../popup-modals/view_on_block_explorer_modal/view_on_block_explorer_modal'
import { PopupModal } from '../popup-modals'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'
import { Button } from './address_actions_menu.style'
import { VerticalDivider, Column } from '../../shared/style'

export interface Props {
  account: BraveWallet.AccountInfo
  children?: React.ReactNode
}

export const AddressActionsMenu = (props: Props) => {
  const { account, children } = props

  // State
  const [showDepositModal, setShowDepositModal] = React.useState(false)
  const [showViewOnExplorerModal, setShowViewOnExplorerModal] =
    React.useState(false)

  // Refs
  const depositModalRef = React.useRef<HTMLDivElement>(null)
  const viewOnExplorerModalRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(
    depositModalRef,
    () => setShowDepositModal(false),
    showDepositModal,
  )
  useOnClickOutside(
    viewOnExplorerModalRef,
    () => setShowViewOnExplorerModal(false),
    showViewOnExplorerModal,
  )

  const handleCopyAddress = () => {
    copyToClipboard(account.address)
    showAlert({
      type: 'success',
      content: getLocale('braveWalletButtonCopied'),
      actions: [],
    })
  }

  return (
    <>
      <ButtonMenu placement='bottom-start'>
        <Button slot='anchor-content'>{children}</Button>
        <leo-menu-item onClick={handleCopyAddress}>
          <Icon name='copy' />
          {getLocale('braveWalletButtonCopy')}
        </leo-menu-item>
        <leo-menu-item onClick={() => setShowDepositModal(true)}>
          <Icon name='qr-code-alternative' />
          {getLocale('braveWalletDepositCryptoButton')}
        </leo-menu-item>
        <leo-menu-item onClick={() => setShowViewOnExplorerModal(true)}>
          <Icon name='web3-blockexplorer' />
          {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
        </leo-menu-item>
      </ButtonMenu>
      {showDepositModal && (
        <PopupModal
          title={getLocale('braveWalletDepositCryptoButton')}
          onClose={() => setShowDepositModal(false)}
          ref={depositModalRef}
        >
          <VerticalDivider />
          <Column
            fullHeight={true}
            fullWidth={true}
            justifyContent='flex-start'
            padding='20px 15px'
          >
            <DepositModal selectedAccount={account} />
          </Column>
        </PopupModal>
      )}
      {showViewOnExplorerModal && (
        <ViewOnBlockExplorerModal
          account={account}
          onClose={() => setShowViewOnExplorerModal(false)}
          ref={viewOnExplorerModalRef}
        />
      )}
    </>
  )
}

export default AddressActionsMenu
