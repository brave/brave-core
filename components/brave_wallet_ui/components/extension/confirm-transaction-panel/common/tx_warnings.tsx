// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Alert from '@brave/leo/react/alert'

// types
import { BraveWallet } from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'

// styles
import { Row } from '../../../shared/style'
import {
  WarningCollapse,
  WarningTitle,
  WarningsList,
  WarningCloseIcon,
  WarningButton,
  DismissButton,
  FullWidth
} from './tx_warnings.styles'

export function TxWarningBanner({
  retrySimulation,
  onDismiss,
  isCritical,
  children
}: React.PropsWithChildren<{
  retrySimulation?: (() => void) | (() => Promise<void>)
  onDismiss?: (() => void) | (() => Promise<void>)
  isCritical?: boolean
}>) {
  // render
  return (
    <FullWidth>
      <Alert
        type={isCritical ? 'error' : 'warning'}
        mode='simple'
      >
        <div slot='icon'>{/* No Icon */}</div>

        {retrySimulation ? (
          <WarningTitle isCritical={isCritical}>
            {getLocale('braveWalletTransactionPreviewFailed')}{' '}
            <Button
              kind='plain'
              onClick={retrySimulation}
            >
              {getLocale('braveWalletButtonRetry')}
            </Button>
          </WarningTitle>
        ) : (
          <WarningTitle
            isBold
            isCritical={isCritical}
          >
            {children}
          </WarningTitle>
        )}

        {onDismiss && (
          <div slot='actions'>
            <WarningButton
              kind='plain'
              onClick={onDismiss}
              isCritical={isCritical}
            >
              <WarningCloseIcon />
            </WarningButton>
          </div>
        )}
      </Alert>
    </FullWidth>
  )
}

export function TransactionWarnings({
  isWarningCollapsed,
  setIsWarningCollapsed,
  warnings,
  onDismiss,
  classifyAs
}: {
  classifyAs: 'risks' | 'issues'
  warnings: Array<Pick<BraveWallet.BlowfishWarning, 'message' | 'severity'>>
  isWarningCollapsed: boolean
  setIsWarningCollapsed?: React.Dispatch<React.SetStateAction<boolean>>
  onDismiss?: (() => void) | (() => Promise<void>)
}): JSX.Element | null {
  // memos
  const hasCriticalWarnings = warnings.some(
    (warning) =>
      warning.severity === BraveWallet.BlowfishWarningSeverity.kCritical
  )

  // no warnings
  if (warnings.length < 1) {
    return null
  }

  // 1 warning
  if (warnings.length === 1) {
    return (
      <FullWidth>
        <TxWarningBanner
          isCritical={hasCriticalWarnings}
          onDismiss={onDismiss}
        >
          {warnings[0].message}
        </TxWarningBanner>
      </FullWidth>
    )
  }

  // multiple warnings
  return (
    <FullWidth>
      <WarningCollapse
        isOpen={!isWarningCollapsed}
        isCritical={hasCriticalWarnings}
        onToggle={
          setIsWarningCollapsed
            ? () => setIsWarningCollapsed((prev) => !prev)
            : undefined
        }
      >
        <WarningTitle
          slot='title'
          isCritical={hasCriticalWarnings}
          isBold
        >
          {getLocale(
            classifyAs === 'risks'
              ? 'braveWalletFoundRisks'
              : 'braveWalletFoundIssues'
          ).replace('$1', warnings.length.toString())}
        </WarningTitle>

        <div>
          <WarningsList>
            {warnings.map((warning) => (
              <li key={warning.message}>{warning.message}</li>
            ))}
          </WarningsList>

          {onDismiss && (
            <Row
              alignItems='flex-start'
              justifyContent='flex-start'
              minWidth={'100%'}
            >
              <DismissButton
                kind='plain'
                onClick={onDismiss}
              >
                {getLocale('braveWalletDismissButton')}
              </DismissButton>
            </Row>
          )}
        </div>
      </WarningCollapse>
    </FullWidth>
  )
}
