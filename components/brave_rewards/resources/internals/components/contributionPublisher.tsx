/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Publisher, PublisherKey } from '../style'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  publisher: RewardsInternals.ContributionPublisher
}

export const ContributionPublisher = (props: Props) => {
  if (!props.publisher) {
    return null
  }

  return (
    <Publisher>
      <PublisherKey>{props.publisher.publisherKey}</PublisherKey>
      <div>
        {getLocale('totalAmount')} {props.publisher.totalAmount} {getLocale('bat')}
      </div>
      <div>
        {getLocale('contributedAmount')} <span data-test-id={'contributed-amount'}>{props.publisher.contributedAmount} {getLocale('bat')}</span>
      </div>
    </Publisher>)
}
