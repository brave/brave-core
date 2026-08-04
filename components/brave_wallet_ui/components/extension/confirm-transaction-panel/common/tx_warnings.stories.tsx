// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { TransactionWarnings, TransactionWarning } from './tx_warnings'

const warnings: TransactionWarning[] = [
  {
    message: 'An example warning',
    severity: 'warning',
  },
  {
    message: 'Another example warning',
    severity: 'warning',
  },
  {
    message: 'Additional example warning',
    severity: 'warning',
  },
  {
    message: 'Yet another example warning',
    severity: 'warning',
  },
  {
    message: 'Yet another example warning',
    severity: 'warning',
  },
  {
    message: 'Yet another example warning',
    severity: 'warning',
  },
  {
    message: 'Yet another example warning',
    severity: 'warning',
  },
  {
    message: 'Yet another example warning',
    severity: 'warning',
  },
  {
    message: 'Yet another example warning',
    severity: 'warning',
  },
]

const criticalWarnings: TransactionWarning[] = [
  {
    message: 'An example critical warning',
    severity: 'critical',
  },
  {
    message: 'Another example critical warning',
    severity: 'critical',
  },
]

export const _TxWarning = {
  render: () => {
    // state
    const [isCollapsed, setIsCollapsed] = React.useState(false)
    return (
      <TransactionWarnings
        classifyAs='issues'
        isWarningCollapsed={isCollapsed}
        setIsWarningCollapsed={setIsCollapsed}
        warnings={[warnings[0]]}
        onDismiss={() => alert('dismiss')}
      />
    )
  },
}

export const _TxWarnings = {
  render: () => {
    // state
    const [isCollapsed, setIsCollapsed] = React.useState(true)
    return (
      <TransactionWarnings
        classifyAs='issues'
        isWarningCollapsed={isCollapsed}
        setIsWarningCollapsed={setIsCollapsed}
        warnings={warnings}
        onDismiss={() => alert('dismiss')}
      />
    )
  },
}

export const _CriticalTxWarning = {
  render: () => {
    // state
    const [isCollapsed, setIsCollapsed] = React.useState(false)
    return (
      <TransactionWarnings
        classifyAs='risks'
        isWarningCollapsed={isCollapsed}
        setIsWarningCollapsed={setIsCollapsed}
        warnings={[criticalWarnings[0]]}
        onDismiss={() => alert('dismiss')}
      />
    )
  },
}

export const _CriticalTxWarnings = {
  render: () => {
    // state
    const [isCollapsed, setIsCollapsed] = React.useState(false)
    return (
      <TransactionWarnings
        classifyAs='risks'
        isWarningCollapsed={isCollapsed}
        setIsWarningCollapsed={setIsCollapsed}
        warnings={criticalWarnings}
        onDismiss={() => alert('dismiss')}
      />
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Warnings',
  component: TransactionWarnings,
}
