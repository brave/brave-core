// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/leo/react/alert'

// utils
import { getLocale } from '../../../../../common/locale'

// styles
import { Row } from '../../../shared/style'
import {
  WarningCollapse,
  AlertTitle,
  WarningsList,
  WarningCloseIcon,
  WarningButton,
  DismissButton,
  FullWidth,
  CollapseTitle,
} from './tx_warnings.styles'

export type TransactionWarningSeverity = 'warning' | 'critical'

export type TransactionWarning = {
  message: string
  severity: TransactionWarningSeverity
}

export function TxWarningBanner({
  onDismiss,
  isCritical,
  children,
}: React.PropsWithChildren<{
  onDismiss?: (() => void) | (() => Promise<void>)
  isCritical?: boolean
}>) {
  // render
  return (
    <FullWidth>
      <Alert type={isCritical ? 'error' : 'warning'}>
        <div slot='icon'>{/* No Icon */}</div>

        <Row justifyContent='space-between'>
          <AlertTitle isCritical={isCritical}>{children}</AlertTitle>
          {onDismiss && (
            <div>
              <WarningButton
                kind='plain'
                onClick={onDismiss}
                isCritical={isCritical}
              >
                <WarningCloseIcon />
              </WarningButton>
            </div>
          )}
        </Row>
      </Alert>
    </FullWidth>
  )
}

export function TransactionWarnings({
  isWarningCollapsed,
  setIsWarningCollapsed,
  warnings,
  onDismiss,
  classifyAs,
}: {
  classifyAs: 'risks' | 'issues'
  warnings: TransactionWarning[]
  isWarningCollapsed: boolean
  setIsWarningCollapsed?: React.Dispatch<React.SetStateAction<boolean>>
  onDismiss?: (() => void) | (() => Promise<void>)
}): JSX.Element | null {
  // memos
  const hasCriticalWarnings = warnings.some(
    (warning) => warning.severity === 'critical',
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
        <CollapseTitle
          slot='title'
          isCritical={hasCriticalWarnings}
        >
          {getLocale(
            classifyAs === 'risks'
              ? 'braveWalletFoundRisks'
              : 'braveWalletFoundIssues',
          ).replace('$1', warnings.length.toString())}
        </CollapseTitle>

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
