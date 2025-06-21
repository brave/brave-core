// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { showAlert } from '@brave/leo/react/alertCenter'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'

// Selectors
import {
  useSafeUISelector, //
} from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

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
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
} from './wellet-menus.style'
import { MenuWrapper, Button } from './address_actions_menu.style'
import { VerticalDivider, Column } from '../../shared/style'

export interface Props {
  account: BraveWallet.AccountInfo
  children?: React.ReactNode
}

export const AddressActionsMenu = (props: Props) => {
  const { account, children } = props

  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isAndroid = useSafeUISelector(UISelectors.isAndroid)

  // State
  const [showMenu, setShowMenu] = React.useState(false)
  const [showDepositModal, setShowDepositModal] = React.useState(false)
  const [showViewOnExplorerModal, setShowViewOnExplorerModal] =
    React.useState(false)

  // Refs
  const menuRef = React.useRef<HTMLDivElement>(null)
  const depositModalRef = React.useRef<HTMLDivElement>(null)
  const viewOnExplorerModalRef = React.useRef<HTMLDivElement>(null)

  // Hooks
  useOnClickOutside(menuRef, () => setShowMenu(false), showMenu)
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
    setShowMenu(false)
    copyToClipboard(account.address)
    showAlert({
      type: 'success',
      content: getLocale('braveWalletButtonCopied'),
      actions: [],
    })
  }

  return (
    <>
      <MenuWrapper ref={menuRef}>
        <Button onClick={() => setShowMenu((prev) => !prev)}>{children}</Button>
        {showMenu && (
          <StyledWrapper
            yPosition={isPanel || isAndroid ? 26 : 46}
            left={0}
          >
            <PopupButton onClick={handleCopyAddress}>
              <ButtonIcon name='copy' />
              <PopupButtonText>
                {getLocale('braveWalletButtonCopy')}
              </PopupButtonText>
            </PopupButton>
            <PopupButton
              onClick={() => {
                setShowDepositModal(true)
                setShowMenu(false)
              }}
            >
              <ButtonIcon name='qr-code-alternative' />
              <PopupButtonText>
                {getLocale('braveWalletDepositCryptoButton')}
              </PopupButtonText>
            </PopupButton>
            <PopupButton
              onClick={() => {
                setShowViewOnExplorerModal(true)
                setShowMenu(false)
              }}
            >
              <ButtonIcon name='web3-blockexplorer' />
              <PopupButtonText>
                {getLocale('braveWalletPortfolioViewOnExplorerMenuLabel')}
              </PopupButtonText>
            </PopupButton>
          </StyledWrapper>
        )}
      </MenuWrapper>
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
