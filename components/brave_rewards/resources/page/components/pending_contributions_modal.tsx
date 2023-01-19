/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { lookupPublisherPlatformName } from '../../shared/lib/publisher_platform'
import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { TokenAmount } from '../../shared/components/token_amount'
import { ExchangeAmount } from '../../shared/components/exchange_amount'
import { isPublisherVerified } from '../../shared/lib/publisher_status'
import { TrashIcon } from './icons/trash_icon'

import * as style from './pending_contributions_modal.style'

function getTypeMessage (type: Rewards.RewardsType) {
  switch (type) {
    case 8: return 'pendingTypetip'
    case 16: return 'pendingTyperecurring'
    default: return 'pendingTypeac'
  }
}

interface Props {
  contributions: Rewards.PendingContribution[]
  exchangeRate: number
  exchangeCurrency: string
  onClose: () => void
  onDelete: (id: number) => void
  onDeleteAll: () => void
}

export function PendingContributionsModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function renderRow (item: Rewards.PendingContribution) {
    const verified = isPublisherVerified(item.status)
    const faviconPath = verified && item.favIcon || item.url
    const expires = new Date(parseInt(item.expirationDate, 10) * 1000)
    const platform = lookupPublisherPlatformName(item.provider)

    const onDelete = () => { props.onDelete(item.id) }

    return (
      <style.contributionItem key={item.id}>
        <style.publisher>
          <NewTabLink href={item.url}>
            <img src={`chrome://favicon/size/64@1x/${faviconPath}`} />
            {item.name}
            {
              platform &&
                <style.platform>
                  &nbsp;{getString('on')} {platform}
                </style.platform>
            }
          </NewTabLink>
        </style.publisher>
        <style.deleteAction>
          <button onClick={onDelete}><TrashIcon /></button>
        </style.deleteAction>
        <style.amount>
          <TokenAmount amount={item.amount} minimumFractionDigits={3} />
          <style.exchangeAmount>
            &nbsp;≈&nbsp;
            <ExchangeAmount
              amount={item.amount}
              rate={props.exchangeRate}
              currency={props.exchangeCurrency}
            />
          </style.exchangeAmount>
        </style.amount>
        <style.contributionType>
          {getString(getTypeMessage(item.type))}
        </style.contributionType>
        <style.contributionDate>
          {
            formatMessage(getString('contributionPendingUntil'), [
              expires.toLocaleDateString()
            ])
          }
        </style.contributionDate>
      </style.contributionItem>
    )
  }

   return (
    <Modal>
      <style.root id='pendingContributionTable'>
        <style.header>
          <style.title>
            {getString('pendingContributions')}
          </style.title>
          <ModalCloseButton onClick={props.onClose} />
        </style.header>
        {
          props.contributions.length > 0
            ? <style.constributionList>
                {props.contributions.map(renderRow)}
                <style.deleteAllAction>
                  <button onClick={props.onDeleteAll}>
                    Remove All
                  </button>
                </style.deleteAllAction>
              </style.constributionList>
            : <style.noContent>
                {getString('pendingContributionEmpty')}
              </style.noContent>
        }
      </style.root>
    </Modal>
  )
}
