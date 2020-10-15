/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, getLocaleWithTag } from '../localeContext'

import {
  Container,
  Title,
  Text,
  LearnMore,
  EnableRewardsButton,
  TermsOfService
} from './style'

interface EnableRewardsPanelProps {
  onEnableRewards: () => void
}

export function EnableRewardsPanel (props: EnableRewardsPanelProps) {
  const locale = React.useContext(LocaleContext)
  const handleClick = () => { props.onEnableRewards() }
  const tags = getLocaleWithTag(locale.get('enableRewardsTerms'))

  return (
    <Container>
      <Title>{locale.get('enableRewardsTitle')}</Title>
      <Text>{locale.get('enableRewardsText')}</Text>
      <LearnMore>
        <a href='javascript:void 0'>{locale.get('enableRewardsLearnMore')}</a>
      </LearnMore>
      <EnableRewardsButton
        text={locale.get('enableRewardsButtonText')}
        size='medium'
        onClick={handleClick}
        type='accent'
        brand='rewards'
      />
      <TermsOfService>
        {tags.beforeTag}
        <a href='javascript: void 0'>{tags.duringTag}</a>
        {tags.afterTag}
      </TermsOfService>
    </Container>
  )
}
