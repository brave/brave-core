// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../common/locale'
import {
  ActionAnchor,
  BasicBox,
  OptionButton,
  PlainButton,
  Text
} from '../../shared/styles'
import * as FTXActions from '../ftx_actions'
import { FTXState } from '../ftx_state'

type Props = {
  ftx: FTXState
  actions: typeof FTXActions
}

export default function PreOptIn (props: Props) {
  // Start as null so that we can use the value from
  // backend as the default (which is region based and is provided asynchronously),
  // but still allow user selection.
  const [userSetIsUSRegion, setUserSetIsUSRegion] = React.useState<boolean | null>(null)
  // Initial value from backend
  let isDotUsSelected = (props.ftx.ftxHost === 'ftx.us')
  if (userSetIsUSRegion !== null) {
    // User has chosen another value
    isDotUsSelected = userSetIsUSRegion
  }

  const startConnect = React.useCallback(() => {
    props.actions.startConnect({ isUS: isDotUsSelected })
  }, [props.actions.startConnect, isDotUsSelected])

  return (
    <>
      <BasicBox isFlex={true} $gap={10} justify='flex-end'>
        <OptionButton
          isSelected={!isDotUsSelected}
          onClick={setUserSetIsUSRegion.bind(undefined, false)}
        >
          .com
        </OptionButton>
        <OptionButton
          isSelected={isDotUsSelected}
          onClick={setUserSetIsUSRegion.bind(undefined, true)}
        >
          .us
        </OptionButton>
      </BasicBox>
      <Text $fontSize={13} weight={600} $pb={6}>
        FTX.{isDotUsSelected ? 'us' : 'com'}
      </Text>
      <Text $fontSize={13} textColor='light' lineHeight={1.5} $pb={21}>
        {getLocale('ftxIntro')}
      </Text>
      <ActionAnchor onClick={props.actions.preOptInViewMarkets.bind(undefined, {})}>
        {getLocale('ftxViewMarkets')}
      </ActionAnchor>
      <PlainButton textColor='light' $m='0 auto' onClick={startConnect}>
        {getLocale('ftxConnect')}
      </PlainButton>
    </>
  )
}
