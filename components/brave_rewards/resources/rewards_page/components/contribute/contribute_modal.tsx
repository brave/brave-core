/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppModelContext, useAppState } from '../../lib/app_model_context'
import { useCallbackWrapper } from '../../lib/callback_wrapper'
import { isSelfCustodyProvider } from '../../../shared/lib/external_wallet'
import { Modal } from '../modal'
import { PaymentForm } from './payment_form'
import { PaymentSelection } from './payment_selection'
import { Sending } from './sending'
import { Success } from './success'
import { TransferError } from './transfer_error'

import { style, backgroundStyle } from './contribute_modal.style'

type ViewType =
  'payment-selection' |
  'payment-form' |
  'sending' |
  'success' |
  'error'

interface Props {
  onClose: () => void
}

export function ContributeModal (props: Props) {
  const model = React.useContext(AppModelContext)
  const wrapCallback = useCallbackWrapper()

  const [creator, externalWallet] = useAppState((state) => [
    state.currentCreator,
    state.externalWallet
  ])

  const [viewType, setViewType] = React.useState<ViewType>(() => {
    if (creator && externalWallet) {
      const hasMatchingCustodialProvider =
        creator.supportedWalletProviders.includes(externalWallet.provider) &&
        !isSelfCustodyProvider(externalWallet.provider)

      if (hasMatchingCustodialProvider && !creator.banner.web3URL) {
        return 'payment-form'
      }
    }
    return 'payment-selection'
  })

  function onSend(amount: number, recurring: boolean) {
    if (!creator) {
      return
    }

    setViewType('sending')

    model
      .sendContribution(creator.site.id, amount, recurring)
      .then(wrapCallback((success) => {
        setViewType(success ? 'success' : 'error')
      }))
  }

  function renderHeader() {
    switch (viewType) {
      case 'sending':
        return null
      case 'success':
      case 'error':
        return <Modal.Header onClose={props.onClose} />
      default:
        return <Modal.Header title='Contribute' onClose={props.onClose} />
    }
  }

  function renderContent() {
    switch (viewType) {
      case 'payment-form':
        return <PaymentForm onCancel={props.onClose} onSend={onSend} />
      case 'payment-selection':
        return (
          <PaymentSelection
            onSelectCustodial={() => setViewType('payment-form')}
            onCancel={props.onClose}
          />
        )
      case 'sending':
        return <Sending />
      case 'success':
        return <Success onClose={props.onClose} />
      case 'error':
        return <TransferError onClose={props.onClose} />
    }
  }

  return (
    <div className={viewType} {...backgroundStyle}>
      <Modal onEscape={props.onClose}>
        <div {...style}>
          {renderHeader()}
          {renderContent()}
        </div>
      </Modal>
    </div>
  )
}
