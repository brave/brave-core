/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'brave-ui/theme'
import { AdsMegaphoneIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../../common/locale'

const Styled = styled.div`
  display: flex;
  flex-direction: row;
  max-width: 96%;
  margin: 0 auto;
  margin-bottom: 16px;
  align-items: center;
`

const Illustration = styled.div`
  flex: 0 0 90px;
  height: 90px;
`

const Text = styled.div`
  margin-left: 16px; // matches padding-bottom from description
  flex: 1;
  line-height: 1.6;
`

const TextP = styled.p`
  margin: 0 0 5px 0;
`

export default function AdsOnboarding () {
  return (
    <Styled>
      <Illustration><AdsMegaphoneIcon /></Illustration>
      <Text>
        <TextP>{getLocale('adsDisabledTextOne')}</TextP>
        <TextP>{getLocale('adsDisabledTextTwo')}</TextP>
      </Text>
    </Styled>
  )
}
