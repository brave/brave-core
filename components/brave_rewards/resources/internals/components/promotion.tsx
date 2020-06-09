/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { formatDate } from '../utils'

interface Props {
  promotion: RewardsInternals.Promotion
}

// Utils
import { getLocale } from '../../../../common/locale'

const getType = (type: number) => {
  switch (type) {
    case 0:
      return getLocale('promotionUGP')
    case 1:
      return getLocale('promotionAds')
  }

  return 'Missing type'
}
const getStatus = (status: number) => {
  switch (status) {
    case 0:
      return getLocale('promotionStatusActive')
    case 1:
      return getLocale('promotionStatusAttested')
    case 4:
      return getLocale('promotionStatusFinished')
    case 5:
      return getLocale('promotionStatusOver')
  }

  return 'Missing status'
}

const getLegacyClaimed = (legacy: boolean) => {
  return legacy
    ? getLocale('promotionLegacyYes')
    : getLocale('promotionLegacyNo')
}

const getClaimData = (claimedAt: number, claimId: string) => {
  if (claimedAt === 0) {
    return null
  }

  return (
    <>
      {getLocale('promotionClaimedAt')}: {formatDate(claimedAt * 1000)}
      <br/>
      {getLocale('promotionClaimId')}: {claimId}
    </>
  )
}

const getExpirationDate = (expiresAt: number) => {
  if (expiresAt === 0) {
    return 0
  }

  return formatDate(expiresAt * 1000)
}

export const Promotion = (props: Props) => (
  <div>
    <h3>{props.promotion.promotionId}</h3>
    {getLocale('promotionStatus')}: {getStatus(props.promotion.status)}
    <br/>
    {getLocale('promotionAmount')}: {props.promotion.amount} {getLocale('bat')}
    <br/>
    {getLocale('promotionType')}: {getType(props.promotion.type)}
    <br/>
    {getLocale('promotionExpiresAt')}: {getExpirationDate(props.promotion.expiresAt)}
    <br/>
    {getLocale('promotionLegacyClaimed')}: {getLegacyClaimed(props.promotion.legacyClaimed)}
    <br/>
    {getLocale('promotionVersion')}: {props.promotion.version}
    <br/>
    {getClaimData(props.promotion.claimedAt, props.promotion.claimId)}
  </div>
)
