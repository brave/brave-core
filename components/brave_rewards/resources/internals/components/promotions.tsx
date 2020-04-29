/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Promotion } from './promotion'

interface Props {
  items: RewardsInternals.Promotion[]
}

// Utils
import { getLocale } from '../../../../common/locale'

export const Promotions = (props: Props) => {
  if (!props.items || props.items.length === 0) {
    return null
  }

  return (
    <>
      <h3>{getLocale('promotions')}</h3>
      {props.items.map((item, index) => (
        <div key={item.promotionId}>
          <Promotion promotion={item} />
          {(index !== props.items.length - 1) ? <hr/> : null}
        </div>
      ))}
    </>
  )
}
