/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { formatString } from '$web-common/formatString'

import { IncomingOffer, ItemKind, TransferItem } from '../api/transfer_api'
import { useTransferApi } from '../api/transfer_api_context'
import { formatBytes } from '../lib/format'
import { getString } from '../lib/strings'
import { itemKindIcon } from './icons'
import { useCountdown } from './use_countdown'

import { style } from './offer_list.style'

function OfferItemRow(props: { item: TransferItem }) {
  const { item } = props
  const isFile = item.kind === ItemKind.kImage || item.kind === ItemKind.kFile
  return (
    <li>
      <Icon name={itemKindIcon(item.kind)} />
      <span className='item-name'>{isFile ? item.name : item.text}</span>
      {isFile && <span className='item-size'>{formatBytes(item.size)}</span>}
    </li>
  )
}

function OfferCard(props: { offer: IncomingOffer }) {
  const api = useTransferApi()
  const { offer } = props
  const seconds = useCountdown(offer.expiryTime)

  return (
    <div
      className='offer'
      role='group'
      aria-label={formatString(getString('BRAVE_TRANSFER_OFFER_FROM'), [
        offer.deviceName,
      ])}
    >
      <div className='offer-header'>
        <span className='offer-from'>
          {formatString(getString('BRAVE_TRANSFER_OFFER_FROM'), [
            <strong key='name'>{offer.deviceName}</strong>,
          ])}
        </span>
        <span className='offer-expiry'>
          {formatString(getString('BRAVE_TRANSFER_OFFER_EXPIRES'), [
            String(seconds),
          ])}
        </span>
      </div>
      <ul className='offer-items'>
        {offer.items.map((item) => (
          <OfferItemRow
            key={item.id}
            item={item}
          />
        ))}
      </ul>
      <div className='offer-actions'>
        <Button
          kind='outline'
          onClick={() => api.respondToOffer([offer.id, false])}
        >
          {getString('BRAVE_TRANSFER_OFFER_DECLINE')}
        </Button>
        <Button
          kind='filled'
          onClick={() => api.respondToOffer([offer.id, true])}
        >
          {getString('BRAVE_TRANSFER_OFFER_ACCEPT')}
        </Button>
      </div>
    </div>
  )
}

export function OfferList() {
  const api = useTransferApi()
  const offers = api.useGetPendingOffersData()

  if (offers.length === 0) {
    return null
  }

  return (
    <section
      data-css-scope={style.scope}
      aria-live='polite'
    >
      <h2>{getString('BRAVE_TRANSFER_OFFERS_TITLE')}</h2>
      {offers.map((offer) => (
        <OfferCard
          key={offer.id}
          offer={offer}
        />
      ))}
    </section>
  )
}
