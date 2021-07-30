/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ContributionPublisher } from './contributionPublisher'
import { formatDate } from '../utils'
import { ContributionPublishersWrapper } from '../style'

interface Props {
  contribution: RewardsInternals.ContributionInfo
}

// Utils
import { getLocale } from '../../../../common/locale'

const getContributionTypeString = (contributionType: number) => {
  switch (contributionType) {
    case 2:
      return getLocale('rewardsTypeAuto')
    case 8:
      return getLocale('rewardsTypeOneTimeTip')
    case 16:
      return getLocale('rewardsTypeRecurringTip')
  }

  return getLocale('rewardsTypeUnknown')
}

const getProcessorString = (processor: number) => {
  switch (processor) {
    case 1:
      return getLocale('processorBraveTokens')
    case 2:
      return getLocale('processorUphold')
    case 3:
      return getLocale('processorBraveUserFunds')
    case 4:
      return getLocale('processorBitflyer')
  }

  return ''
}

const getContributionStepString = (step: number) => {
  switch (step) {
    case -7:
      return getLocale('contributionStepRetryCount')
    case -6:
      return getLocale('contributionStepAutoContributeOff')
    case -5:
      return getLocale('contributionStepRewardsOff')
    case -4:
      return getLocale('contributionStepAutoContributeTableEmpty')
    case -3:
      return getLocale('contributionStepNotEnoughFunds')
    case -2:
      return getLocale('contributionStepFailed')
    case -1:
      return getLocale('contributionStepCompleted')
    case 0:
      return getLocale('contributionStepNo')
    case 1:
      return getLocale('contributionStepStart')
    case 2:
      return getLocale('contributionStepPrepare')
    case 3:
      return getLocale('contributionStepReserve')
    case 4:
      return getLocale('contributionStepExternalTransaction')
    case 5:
      return getLocale('contributionStepCreds')
  }

  return getLocale('contributionStepNo')
}

export const Contribution = (props: Props) => (
  <div>
    <h3>{props.contribution.id}</h3>
    <div>
      {getLocale('contributionCreatedAt')} {formatDate(props.contribution.createdAt * 1000)}
    </div>
    <div>
      {getLocale('contributionType')} {getContributionTypeString(props.contribution.type)}
    </div>
    <div>
      {getLocale('amount')} {props.contribution.amount} {getLocale('bat')}
    </div>
    <div>
      {getLocale('contributionStep')} {getContributionStepString(props.contribution.step)}
    </div>
    <div>
      {getLocale('retryCount')} {props.contribution.retryCount}
    </div>
    <div>
      {getLocale('contributionProcessor')} {getProcessorString(props.contribution.processor)}
    </div>
    <ContributionPublishersWrapper data-test-id={'publisher-wrapper'}>
      {props.contribution.publishers.map((item: RewardsInternals.ContributionPublisher) => (
        <ContributionPublisher publisher={item} key={`publisher-${item.publisherKey}`} />
      ))}
    </ContributionPublishersWrapper>
  </div>
)
