// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { getLocale } from '$web-common/locale'

import { NavButton, Panel } from '../..'
import {
  ErrorIcon,
  Title,
  ErrorDetailTitle,
  ErrorDetailContent,
  ErrorDetailContentContainer
} from './failed.style'
import { ButtonRow, TransactionStatusDescription } from '../common/common.style'
import { PopupModal } from '../../popup-modals'

interface Props {
  headerTitle: string
  isPrimaryCTADisabled: boolean
  errorDetailTitle: string
  errorDetailContent?: string | undefined
  onClose: () => void
  onClickPrimaryCTA: () => void
}

export const TransactionFailed = (props: Props) => {
  const {
    headerTitle,
    errorDetailTitle,
    errorDetailContent,
    isPrimaryCTADisabled,
    onClose,
    onClickPrimaryCTA
  } = props

  const [showErrorCodeModal, setShowErrorCodeModal] = React.useState<boolean>(false)

  return (
    <Panel navAction={onClose} title={headerTitle} headerStyle='slim'>
      <ErrorIcon />
      <Title>{getLocale('braveWalletTransactionFailedTitle')}</Title>
      <TransactionStatusDescription>
        {getLocale('braveWalletTransactionFailedDescription')}
      </TransactionStatusDescription>
      <ButtonRow>
        {errorDetailContent && (
          <NavButton
            buttonType='secondary'
            text={getLocale('braveWalletTransactionFailedViewErrorCTA')}
            onSubmit={() => setShowErrorCodeModal(true)}
          />
        )}
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletButtonClose')}
          onSubmit={onClickPrimaryCTA}
          disabled={isPrimaryCTADisabled}
        />
      </ButtonRow>

      {showErrorCodeModal && errorDetailContent && (
        <ErrorDetail
          title={errorDetailTitle}
          content={errorDetailContent}
          onClose={() => setShowErrorCodeModal(false)}
        />
      )}
    </Panel>
  )
}

interface ModalProps {
  title: string
  content: string
  onClose: () => void
}

const ErrorDetail = (props: ModalProps) => {
  const { content, title, onClose } = props

  return (
    <PopupModal title={getLocale('braveWalletTransactionFailedModalTitle')} onClose={onClose}>
      <ErrorDetailTitle>{title}</ErrorDetailTitle>

      <ErrorDetailContentContainer>
        <ErrorDetailContent>{content}</ErrorDetailContent>
      </ErrorDetailContentContainer>

      <ButtonRow>
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletButtonClose')}
          onSubmit={onClose}
        />
      </ButtonRow>
    </PopupModal>
  )
}
