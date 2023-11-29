// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'

// types
import { BraveWallet } from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  translateSimulationWarning //
} from '../../../../utils/tx-simulation-utils'

// components
import { TxWarningBanner } from './tx_warning_banner'

// styles
import { FullWidth } from '../../../shared/style'
import { WarningsList } from '../style'
import {
  CriticalWarningIcon,
  WarningCollapse,
  WarningTriangleFilledIcon
} from '../confirm_simulated_tx_panel.styles'

export function SimulationWarnings({
  hasCriticalWarnings,
  isWarningCollapsed,
  setIsWarningCollapsed,
  txSimulation
}: {
  txSimulation:
    | BraveWallet.EVMSimulationResponse
    | BraveWallet.SolanaSimulationResponse
  isWarningCollapsed: boolean
  hasCriticalWarnings: boolean
  setIsWarningCollapsed: React.Dispatch<React.SetStateAction<boolean>>
}): JSX.Element | null {
  // no warnings
  if (txSimulation.warnings.length < 1) {
    return null
  }

  // 1 warning
  if (txSimulation.warnings.length === 1) {
    return (
      <FullWidth>
        <TxWarningBanner isCritical={hasCriticalWarnings}>
          {translateSimulationWarning(txSimulation.warnings[0])}
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
        onToggle={() => setIsWarningCollapsed((prev) => !prev)}
        title={
          hasCriticalWarnings
            ? getLocale('braveWalletRiskOfLossAction')
            : getLocale('braveWalletFoundIssues').replace(
                '$1',
                txSimulation.warnings.length.toString()
              )
        }
      >
        <div slot='icon'>
          {hasCriticalWarnings ? (
            <CriticalWarningIcon />
          ) : (
            <WarningTriangleFilledIcon />
          )}
        </div>

        <WarningsList>
          {txSimulation.warnings.map((warning) => (
            <li key={warning.message}>{translateSimulationWarning(warning)}</li>
          ))}
        </WarningsList>
      </WarningCollapse>
    </FullWidth>
  )
}
