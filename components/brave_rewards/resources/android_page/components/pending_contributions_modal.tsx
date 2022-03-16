/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Profile from '../../ui/components/profile/index'
import { DetailRow, getTypeMessage } from '../../ui/components/tablePending/index'
import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { NewTabLink } from '../../shared/components/new_tab_link'
import Tokens from '../../ui/components/tokens/index'
import { TrashIcon } from './icons/trash_icon'

import * as style from './pending_contributions_modal.style'

interface Props {
  rows: DetailRow[]
  onClose: () => void
}

export function PendingContributionsModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  function renderRow (row: DetailRow) {
    /* Each row is a grid
       ---------------------------------------------
       |   | 1(29px) | 2(1fr) |  3(1fr)  | 4(14px) |
       ---------------------------------------------
       | 1 |  Icon     Site              |     |_| |
       | 2 |         | Pending Amount              |
       | 3 |         | Type  |                Date |
       ---------------------------------------------
    */
     return (
      <style.constributionsListItem>
        <style.contributionReceiver>
          <NewTabLink href={row.url}>
            <Profile
              title={row.profile.name}
              provider={row.profile.provider}
              verified={row.profile.verified}
              src={row.profile.src}
              type='mobile'
            />
          </NewTabLink>
        </style.contributionReceiver>
        <style.contributionDelete>
          <button onClick={row.onRemove}><TrashIcon /></button>
        </style.contributionDelete>
        <style.contributionAmount>
          <Tokens
            value={row.amount.tokens}
            converted={row.amount.converted}
            size={'small'}
            showApproxSign={true}
          />
        </style.contributionAmount>
        <style.contributionType>
          {getString(getTypeMessage(row.type))}
        </style.contributionType>
        <style.contributionDate>
          {
            formatMessage(getString('contributionPendingUntil'), [
              row.date
            ])
          }
        </style.contributionDate>
      </style.constributionsListItem>
    )
  }

   return (
    <Modal>
      <style.root>
        <style.header>
          <style.title>
            {getString('pendingContributions')}
          </style.title>
          <ModalCloseButton onClick={props.onClose} />
        </style.header>
        { props.rows && props.rows.length > 0
          ? <style.constributionsList>
            {props.rows.map(renderRow)}
          </style.constributionsList>
          : <style.noContent>
            {getString('pendingContributionEmpty')}
          </style.noContent>
        }
      </style.root>
    </Modal>
  )
}
