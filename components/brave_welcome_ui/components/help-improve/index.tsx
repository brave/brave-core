// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import Button from '$web-components/button'
import { WelcomeBrowserProxyImpl } from '../../api/welcome_browser_proxy'
import { ViewType } from '../../state/component_types'
import { getLocale, splitStringForTag } from '$web-common/locale'

interface InputCheckboxProps {
  onChange?: (e: React.ChangeEvent<HTMLInputElement>) => void
  labelText: string
  id?: string
  isChecked: boolean
}

function InputCheckbox (props: InputCheckboxProps) {
  return (
    <label className="item">
      <input
        type="checkbox"
        checked={props.isChecked}
        onChange={props.onChange}
      />
      <div>
        {props.labelText}
      </div>
    </label>
  )
}

function HelpImprove () {
  const [isMetricsReportingEnabled, setMetricsReportingEnabled] = React.useState(true)
  const [isP3AEnabled, setP3AEnabled] = React.useState(true)

  const handleP3AChange = () => {
    setP3AEnabled(!isP3AEnabled)
  }

  const handleMetricsReportingChange = () => {
    setMetricsReportingEnabled(!isMetricsReportingEnabled)
  }

  const handleFinish = () => {
    WelcomeBrowserProxyImpl.getInstance().setP3AEnabled(isP3AEnabled)
    WelcomeBrowserProxyImpl.getInstance().setMetricsReportingEnabled(isMetricsReportingEnabled)
    WelcomeBrowserProxyImpl.getInstance().recordP3A({ currentScreen: ViewType.HelpImprove, isFinished: true, isSkipped: false })
    window.open('chrome://newtab', '_self')
  }

  const handleOpenSettingsPage = () => {
    WelcomeBrowserProxyImpl.getInstance().openSettingsPage()
  }

  const changeSettingsNote = splitStringForTag(getLocale('braveWelcomeChangeSettingsNote'))
  const readPrivacyPolicy = splitStringForTag(getLocale('braveWelcomePrivacyPolicyNote'))

  return (
    <S.MainBox>
      <div className="view-header-box">
        <div className="view-details">
          <h1 className="view-title">{getLocale('braveWelcomeHelpImproveBraveTitle')}</h1>
          <p className="view-desc">{getLocale('braveWelcomeHelpImproveBraveDesc')}</p>
        </div>
      </div>
      <S.Grid>
        <div className="list">
          <InputCheckbox
            id="metrics"
            onChange={handleMetricsReportingChange}
            isChecked={isMetricsReportingEnabled}
            labelText={getLocale('braveWelcomeSendReportsLabel')}
          />
          <InputCheckbox
            id="p3a"
            onChange={handleP3AChange}
            isChecked={isP3AEnabled}
            labelText={getLocale('braveWelcomeSendInsightsLabel')}
          />
        </div>
      </S.Grid>
      <S.ActionBox>
        <div className="box-center">
          <Button
            isPrimary={true}
            onClick={handleFinish}
            scale="jumbo"
          >
            {getLocale('braveWelcomeFinishButtonLabel')}
          </Button>
          <S.FootNote>
            {changeSettingsNote.beforeTag}
            <a href="brave://settings/privacy" onClick={handleOpenSettingsPage}>
              {changeSettingsNote.duringTag}
            </a>
            {changeSettingsNote.afterTag}
            <span>
              {readPrivacyPolicy.beforeTag}
              <a href="https://brave.com/privacy/browser" target="_blank">
                {readPrivacyPolicy.duringTag}
              </a>
              {readPrivacyPolicy.afterTag}
            </span>
          </S.FootNote>
        </div>
      </S.ActionBox>
    </S.MainBox>
  )
}

export default HelpImprove
