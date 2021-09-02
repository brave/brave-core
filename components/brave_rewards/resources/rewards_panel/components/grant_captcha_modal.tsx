/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { GrantCaptchaInfo } from '../lib/interfaces'
import { GrantCaptchaChallenge } from './grant_captcha_challenge'
import { Modal, ModalCloseButton } from '../../shared/components/modal'
import { TokenAmount } from '../../shared/components/token_amount'

import * as style from './grant_captcha_modal.style'

interface Props {
  grantCaptchaInfo: GrantCaptchaInfo
  onSolve: (solution: { x: number, y: number }) => void
  onClose: () => void
}

export function GrantCaptchaModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { grantCaptchaInfo } = props
  const { grantInfo } = grantCaptchaInfo

  function renderChallenge (message: string) {
    return (
      <>
        <style.header>{getString(message)}</style.header>
        <GrantCaptchaChallenge
          grantCaptchaInfo={grantCaptchaInfo}
          onSolve={props.onSolve}
        />
      </>
    )
  }

  function renderPassed () {
    const dateFormatter = Intl.DateTimeFormat(undefined, {
      year: 'numeric',
      month: 'numeric',
      day: 'numeric'
    })

    function getStrings () {
      switch (grantInfo.source) {
        case 'ads':
          return {
            title: getString('grantCaptchaPassedTitleAds'),
            text: getString('grantCaptchaPassedTextAds'),
            amountLabel: getString('grantCaptchaAmountAds')
          }
        case 'ugp':
          return {
            title: getString('grantCaptchaPassedTitleUGP'),
            text: getString('grantCaptchaPassedTextUGP'),
            amountLabel: getString('grantCaptchaAmountUGP')
          }
      }
    }

    const { title, text, amountLabel } = getStrings()

    return (
      <>
        <style.header>{title}</style.header>
        <style.text>{text}</style.text>
        <style.summary>
          <style.summaryItem>
            {amountLabel}
            <style.summaryValue>
              <TokenAmount amount={grantInfo.amount} />
            </style.summaryValue>
          </style.summaryItem>
          {
            grantInfo.expiresAt !== null &&
              <style.summaryItem>
                {getString('grantCaptchaExpiration')}
                <style.summaryValue>
                  {dateFormatter.format(grantInfo.expiresAt)}
                </style.summaryValue>
              </style.summaryItem>
          }
        </style.summary>
        <style.okButton>
          <button onClick={props.onClose}>{getString('ok')}</button>
        </style.okButton>
      </>
    )
  }

  function renderError () {
    return (
      <>
        <style.header>
          {getString('grantCaptchaErrorTitle')}
        </style.header>
        <style.text>
          {getString('grantCaptchaErrorText')}
        </style.text>
        <style.okButton>
          <button onClick={props.onClose}>{getString('ok')}</button>
        </style.okButton>
      </>
    )
  }

  function renderContent (): React.ReactNode {
    switch (props.grantCaptchaInfo.status) {
      case 'pending': return renderChallenge('grantCaptchaTitle')
      case 'passed': return renderPassed()
      case 'failed': return renderChallenge('grantCaptchaFailedTitle')
      case 'error': return renderError()
    }
  }

  return (
    <Modal>
      <style.root>
        <ModalCloseButton onClick={props.onClose} />
        {renderContent()}
      </style.root>
    </Modal>
  )
}
