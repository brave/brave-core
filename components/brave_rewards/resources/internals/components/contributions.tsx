/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Contribution } from './contribution'
import { ButtonWrapper } from '../style'
import { Button } from 'brave-ui/components'

interface Props {
  items: RewardsInternals.CurrentReconcile[]
  onGet: () => void
}

// Utils
import { getLocale } from '../../../../common/locale'

export const Contributions = (props: Props) => {
  if (!props.items || props.items.length === 0) {
    return null
  }

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
      {props.items.map((item, index) => (
        <>
          <div>
            {getLocale('currentReconcile')} {index + 1}
            <Contribution reconcile={item || ''} />
          </div>
          <hr/>
        </>
      ))}
    </>
  )
}
