/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'
import Tooltip from '@brave/leo/react/tooltip'

import { formatMessage } from '../../../shared/lib/locale_context'
import { formatEarningsRange } from '../../lib/formatters'
import { useLocaleContext } from '../../lib/locale_strings'
import { AdType } from '../../lib/app_model'
import { AppModelContext, useAppState } from '../../lib/app_model_context'
import { Modal } from '../modal'
import { NewTabLink } from '../../../shared/components/new_tab_link'

import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './ads_settings_modal.style'

const payoutDateFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long',
  day: 'numeric'
})

const adsPerHourOptions = [0, 1, 2, 3, 4, 5, 10]

interface Props {
  onClose: () => void
}

export function AdsSettingsModal(props: Props) {
  const model = React.useContext(AppModelContext)
  const { getString } = useLocaleContext()

  const [adsInfo, externalWallet] = useAppState((state) => [
    state.adsInfo,
    state.externalWallet
  ])

  if (!adsInfo) {
    return null
  }

  function onToggleChange(adType: AdType) {
    return (detail: { checked: boolean }) => {
      model.setAdTypeEnabled(adType, detail.checked)
    }
  }

  function adsPerHourOptionText(n: number) {
    if (n === 0) {
      return getString('adsSettingsAdsPerHourNoneText')
    }
    return formatMessage(getString('adsSettingsAdsPerHourText'), [n])
  }

  function onNotificationAdsPerHourChange(
    event: React.FormEvent<HTMLSelectElement>
  ) {
    const value = Number(event.currentTarget.value) || 0
    model.setNotificationAdsPerHour(value)
  }

  function onSubdivisionChange(event: React.ChangeEvent<HTMLSelectElement>) {
    model.setAdsSubdivision(event.target.value)
  }

  function renderSubdivisions() {
    if (!adsInfo || !adsInfo.shouldAllowSubdivisionTargeting) {
      return null
    }
    if (adsInfo.availableSubdivisions.length === 0) {
      return null
    }
    let autoNameSuffix = ''
    for (const { code, name } of adsInfo.availableSubdivisions) {
      if (adsInfo.autoDetectedSubdivision === code) {
        autoNameSuffix = ` (${name})`
        break
      }
    }
    return (
      <section className='subdivisions'>
        <div className='subdivision-row'>
          <label>{getString('adsSettingsSubdivisionLabel')}</label>
          <select
            value={adsInfo.currentSubdivision}
            onChange={onSubdivisionChange}
          >
            <option value='DISABLED'>
              {getString('adsSettingsSubdivisionDisabledLabel')}
            </option>
            <option value='AUTO'>
              {getString('adsSettingsSubdivisionAutoLabel')}
              {autoNameSuffix}
            </option>
            {
              adsInfo.availableSubdivisions.map(({ code, name }) =>
                <option key={code} value={code}>{name}</option>
              )
            }
          </select>
        </div>
        <div>
          {getString('adsSettingsSubdivisionText')}
          {' '}
          <NewTabLink href={urls.adsLearnMoreURL}>
            {getString('learnMoreLink')}
          </NewTabLink>
        </div>
      </section>
    )
  }

  return (
    <Modal onEscape={props.onClose}>
      <Modal.Header
        title={getString('adsSettingsTitle')}
        onClose={props.onClose}
      />
      <div {...style}>
        <p className='description'>
          {getString('adsSettingsText')}
          {' '}
          <NewTabLink href={urls.adsLearnMoreURL}>
            {getString('learnMoreLink')}
          </NewTabLink>
        </p>
        <section className='summary'>
          <div className='row'>
            <span>
              {getString('adsSettingsEarningsLabel')}
            </span>
            <span className='value'>
              {
                formatEarningsRange(
                    adsInfo.minEarningsThisMonth,
                    adsInfo.maxEarningsThisMonth)
              } BAT
            </span>
          </div>
          <div className='row'>
            <span>
              {getString('adsSettingsPayoutDateLabel')}
            </span>
            <span className='value'>
              {payoutDateFormatter.format(new Date(adsInfo.nextPaymentDate))}
            </span>
          </div>
          <div className='row'>
            <span>
              {getString('adsSettingsTotalAdsLabel')}
            </span>
            <span className='value'>
              {adsInfo.adsReceivedThisMonth}
            </span>
          </div>
        </section>
        <section className='ad-types'>
          <div className='header'>
            <span>{getString('adsSettingsAdTypeTitle')}</span>
            <span>{getString('adsSettingsAdViewsTitle')}</span>
          </div>
          <div className='row'>
            <Toggle
              checked={adsInfo.adsEnabled['new-tab-page']}
              onChange={onToggleChange('new-tab-page')}
            />
            <span className='name'>
              {getString('adTypeNewTabPageLabel')}
            </span>
            <span>
              {adsInfo.adTypesReceivedThisMonth['new-tab-page']}
            </span>
          </div>
          <div className='row'>
            <Toggle
              checked={adsInfo.adsEnabled.notification}
              onChange={onToggleChange('notification')}
            />
            <span className='name'>
              {getString('adTypeNotificationLabel')}
              <select
                value={adsInfo.notificationAdsPerHour}
                onChange={onNotificationAdsPerHourChange}
              >
                {
                  adsPerHourOptions
                    .filter((n) => n || n === adsInfo.notificationAdsPerHour)
                    .map((n) => (
                      <option key={n} value={n}>
                        {adsPerHourOptionText(n)}
                      </option>
                    ))
                }
              </select>
            </span>
            <span>
              {adsInfo.adTypesReceivedThisMonth.notification}
            </span>
          </div>
          <div className='row'>
            <Toggle
              checked={adsInfo.adsEnabled['search-result']}
              onChange={onToggleChange('search-result')}
            />
            <span className='name'>
              {getString('adTypeSearchResultLabel')}
              <Tooltip mode='default'>
                <Icon name='info-outline' />
                <div slot='content'>
                  {
                    externalWallet &&
                      <p>
                        {getString('adsSettingsSearchConnectedTooltip')}
                      </p>
                  }
                  <p>
                    {
                      formatMessage(getString('adsSettingsSearchTooltip'), {
                        tags: {
                          $1: (content) => (
                            <NewTabLink
                              key='link'
                              href={urls.braveSearchURL}
                            >
                              {content}
                            </NewTabLink>
                          )
                        }
                      })
                    }
                  </p>
                </div>
              </Tooltip>
            </span>
            <span>
              {adsInfo.adTypesReceivedThisMonth['search-result']}
            </span>
          </div>
          <div className='row'>
            <Toggle
              checked={adsInfo.adsEnabled['inline-content']}
              disabled
            />
            <span className='name'>
              {getString('adTypeInlineContentLabel')}
              <Tooltip mode='default'>
                <Icon name='info-outline' />
                <div slot='content'>
                  <p>
                    {
                      adsInfo.adsEnabled['inline-content']
                          ? getString('adsSettingsNewsOnTooltip')
                          : getString('adsSettingsNewsOffTooltip')
                    }
                  </p>
                </div>
              </Tooltip>
            </span>
            <span>
              {adsInfo.adTypesReceivedThisMonth['inline-content']}
            </span>
          </div>
        </section>
        {renderSubdivisions()}
      </div>
    </Modal>
  )
}
