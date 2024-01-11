/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { formatDate } from '../utils'

import * as mojom from '../../shared/lib/mojom'

interface Props {
  state: RewardsInternals.State
}

const getKeyInfoSeedValidString = (isValid: boolean) => {
  if (isValid) {
    return getLocale('valid')
  }

  return getLocale('invalid')
}

const getEnvironmentString = (environment: mojom.Environment | undefined) => {
  switch (environment) {
    case mojom.Environment.kStaging:
      return 'Staging'
    case mojom.Environment.kProduction:
      return 'Production'
    case mojom.Environment.kDevelopment:
      return 'Development'
    default:
      return getLocale('notSet')
  }
}

const getInfo = (state: RewardsInternals.State) => {
  return (
    <>
      <div>
        {getLocale('keyInfoSeed')} {getKeyInfoSeedValidString(state.info.isKeyInfoSeedValid || false)}
      </div>
      <div>
        {getLocale('walletPaymentId')} {state.info.walletPaymentId || ''}
      </div>
      {
        state.info.walletCreationEnvironment !== undefined && (
          <div>
            {getLocale('walletCreationEnvironment')} {getEnvironmentString(state.info.walletCreationEnvironment)}
          </div>
        )
      }
      {
        state.info.walletCreationEnvironment !== undefined &&
        state.info.walletCreationEnvironment !== state.environment && (
          <div style={{ color: 'red' }} >
            {getLocale('currentEnvironment')} {getEnvironmentString(state.environment)}
          </div>
        )
      }
      <div>
        {getLocale('bootStamp')} {formatDate(state.info.bootStamp * 1000)}
      </div>
      <div>
        {getLocale('rewardsCountry')} {state.info.declaredGeo || getLocale('notSet')}
      </div>
    </>)
}

export const WalletInfo = (props: Props) => {
  const info = props.state.info

  return (
    <>
      <h3>{getLocale('walletInfo')}</h3>
      {
        info.bootStamp
          ? getInfo(props.state)
          : <div>
            {getLocale('walletNotCreated')}
          </div>
      }

    </>
  )
}
