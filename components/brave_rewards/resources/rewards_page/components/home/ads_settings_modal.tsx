/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Toggle from '@brave/leo/react/toggle'

import { formatMessage } from '../../../shared/lib/locale_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { AdType } from '../../lib/app_state'
import { AppModelContext, useAppState } from '../../lib/app_model_context'
import { Modal } from '../common/modal'
import { NewTabLink } from '../../../shared/components/new_tab_link'

import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './ads_settings_modal.style'

const payoutDateFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long',
  day: 'numeric',
})

const adsPerHourOptions = [0, 1, 2, 3, 4, 5, 10]

interface Props {
  onClose: () => void
}

export function AdsSettingsModal(props: Props) {
  const model = React.useContext(AppModelContext)
  const { getString } = useLocaleContext()

  const adsInfo = useAppState((state) => state.adsInfo)
  const externalWallet = useAppState((state) => state.externalWallet)

  if (!adsInfo) {
    return null
  }

  const adsReceivedThisMonth = Object.values(
    adsInfo.adTypesReceivedThisMonth,
  ).reduce((prev, current) => prev + current, 0)

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
    event: React.FormEvent<HTMLSelectElement>,
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
            {adsInfo.availableSubdivisions.map(({ code, name }) => (
              <option
                key={code}
                value={code}
              >
                {name}
              </option>
            ))}
          </select>
        </div>
        <div>
          {getString('adsSettingsSubdivisionText')}{' '}
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
      <div data-css-scope={style.scope}>
        {externalWallet && (
          <p className='description'>
            {getString('adsSettingsText')}{' '}
            <NewTabLink href={urls.adsLearnMoreURL}>
              {getString('learnMoreLink')}
            </NewTabLink>
          </p>
        )}
        <section className='summary'>
          {externalWallet && (
            <div className='row'>
              <span>{getString('adsSettingsPayoutDateLabel')}</span>
              <span className='value'>
                {payoutDateFormatter.format(new Date(adsInfo.nextPaymentDate))}
              </span>
            </div>
          )}
          <div className='row'>
            <span>{getString('adsSettingsTotalAdsLabel')}</span>
            <span className='value'>{adsReceivedThisMonth}</span>
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
            <span className='name'>{getString('adTypeNewTabPageLabel')}</span>
            <span>{adsInfo.adTypesReceivedThisMonth['new-tab-page']}</span>
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
                {adsPerHourOptions
                  .filter((n) => n || n === adsInfo.notificationAdsPerHour)
                  .map((n) => (
                    <option
                      key={n}
                      value={n}
                    >
                      {adsPerHourOptionText(n)}
                    </option>
                  ))}
              </select>
            </span>
            <span>{adsInfo.adTypesReceivedThisMonth.notification}</span>
          </div>
        </section>
        {renderSubdivisions()}
      </div>
    </Modal>
  )
}
