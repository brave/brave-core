/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Contribution } from './contribution'
import { ButtonWrapper } from '../style'
import { Button } from 'brave-ui/components'

interface Props {
  items: RewardsInternals.ContributionInfo[]
  onGet: () => void
}

// Utils
import { getLocale } from '../../../../common/locale'

const getItems = (items: RewardsInternals.ContributionInfo[]) => {
  if (!items || items.length === 0) {
    return null
  }

  return items
    .sort((first, second) => {
      return second.createdAt - first.createdAt
    })
    .map((item, index) => (
      <div key={item.id}>
        <Contribution contribution={item || ''} />
        {(index !== items.length - 1) ? <hr/> : null}
      </div>
    ))
}

export const Contributions = (props: Props) => {
  return (
    <>
      <ButtonWrapper>
        <Button
          text={getLocale('refreshButton')}
          size={'medium'}
          type={'accent'}
          onClick={props.onGet}
        />
      </ButtonWrapper>
      {getItems(props.items)}
    </>
  )
}
