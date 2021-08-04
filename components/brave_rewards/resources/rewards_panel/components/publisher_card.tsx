/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { HostContext, useHostListener } from '../lib/host_context'
import { MonthlyTipAction } from '../lib/interfaces'
import { ToggleButton } from './toggle_button'
import { MonthlyTipView } from './monthly_tip_view'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { VerifiedIcon } from './icons/verified_icon'

import * as styles from './publisher_card.style'

const unverifiedLearnMoreURL = 'https://brave.com/faq/#unclaimed-funds'

export function PublisherCard () {
  const { getString } = React.useContext(LocaleContext)
  const host = React.useContext(HostContext)

  const [publisherInfo, setPublisherInfo] =
    React.useState(host.state.publisherInfo)
  const [hidePublisherUnverifiedNote, setHidePublisherUnverifiedNote] =
    React.useState(host.state.hidePublisherUnverifiedNote)
  const [externalWallet, setExternalWallet] =
    React.useState(host.state.externalWallet)

  useHostListener(host, (state) => {
    setPublisherInfo(state.publisherInfo)
    setHidePublisherUnverifiedNote(host.state.hidePublisherUnverifiedNote)
    setExternalWallet(state.externalWallet)
  })

  if (!publisherInfo) {
    return null
  }

  function renderStatusMessage () {
    if (!publisherInfo) {
      return null
    }

    if (publisherInfo.registered) {
      return (
        <styles.verified>
          <VerifiedIcon />{getString('verifiedCreator')}
        </styles.verified>
      )
    }

    return (
      <styles.unverified>
        <VerifiedIcon />{getString('unverifiedCreator')}
      </styles.unverified>
    )
  }

  function renderUnverifiedNote () {
    if (!publisherInfo || hidePublisherUnverifiedNote) {
      return null
    }

    const walletProviderNotSupported =
      externalWallet &&
      !publisherInfo.supportedWalletProviders.includes(externalWallet.provider)

    if (!publisherInfo.registered || walletProviderNotSupported) {
      const noteText = getString(walletProviderNotSupported
        ? 'providerNotSupportedNote'
        : 'unverifiedNote')

      return (
        <styles.unverifiedNote>
          <strong>{getString('note')}:</strong>&nbsp;
          {noteText}&nbsp;
          {
            formatMessage(getString('unverifiedLinks'), {
              tags: {
                $1: (content) =>
                  <NewTabLink href={unverifiedLearnMoreURL} key='learn-more'>
                    {content}
                  </NewTabLink>,
                $3: (content) =>
                  <a href='#' key='hide' onClick={hideVerifiedNote}>
                    {content}
                  </a>
              }
            })
          }
        </styles.unverifiedNote>
      )
    }

    return null
  }

  function hideVerifiedNote (evt: React.UIEvent) {
    evt.preventDefault()
    host.hidePublisherUnverifiedNote()
  }

  function onRefreshClick (evt: React.UIEvent) {
    evt.preventDefault()
    host.refreshPublisherStatus()
  }

  function monthlyTipHandler (action: MonthlyTipAction) {
    return () => {
      host.handleMonthlyTipAction(action)
    }
  }

  return (
    <styles.root>
      <styles.heading>
        {
          publisherInfo.icon &&
            <styles.icon>
              <img src={publisherInfo.icon} />
            </styles.icon>
        }
        <styles.name>
          {publisherInfo.name}
          <styles.status>
            {renderStatusMessage()}
            <styles.refreshStatus>
              <a href='#' onClick={onRefreshClick}>
                {getString('refreshStatus')}
              </a>
            </styles.refreshStatus>
          </styles.status>
        </styles.name>
      </styles.heading>
      {renderUnverifiedNote()}
      <styles.attention>
        <div>{getString('attention')}</div>
        <div className='value'>
          {(publisherInfo.attentionScore * 100).toFixed(0)}%
        </div>
      </styles.attention>
      <styles.contribution>
        <styles.autoContribution>
          <div>{getString('includeInAutoContribute')}</div>
          <div>
            <ToggleButton
              checked={publisherInfo.autoContributeEnabled}
              onChange={host.setIncludeInAutoContribute}
            />
          </div>
        </styles.autoContribution>
        <styles.monthlyContribution>
          <div>{getString('monthlyContribution')}</div>
          <div>
            <MonthlyTipView
              publisherInfo={publisherInfo}
              onUpdateClick={monthlyTipHandler('update')}
              onCancelClick={monthlyTipHandler('cancel')}
            />
          </div>
        </styles.monthlyContribution>
      </styles.contribution>
      <styles.tipAction>
        <button>
          {getString('sendTip')}
        </button>
      </styles.tipAction>
    </styles.root>
  )
}
