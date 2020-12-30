/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { formatDate } from '../utils'

interface Props {
  state: RewardsInternals.State
}

const getKeyInfoSeedValidString = (isValid: boolean) => {
  if (isValid) {
    return getLocale('valid')
  }

  return getLocale('invalid')
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
      <div>
        {getLocale('bootStamp')} {formatDate(state.info.bootStamp * 1000)}
      </div>
    </>)
}

export const WalletInfo = (props: Props) => {
  const info = props.state.info

  return (
    <>
      <h3>{getLocale('walletInfo')}</h3>
      {
        info.bootStamp === 0
        ? <div>
          {getLocale('walletNotCreated')}
        </div>
        : getInfo(props.state)
      }

    </>
  )
}
