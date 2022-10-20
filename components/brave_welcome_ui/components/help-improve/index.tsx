// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import Button from '$web-components/button'
import { BravePrivacyBrowserProxyImpl } from '../../api/privacy_data_browser'
import { getLocale } from '$web-common/locale'

interface InputCheckboxProps {
  onChange?: (e: React.ChangeEvent<HTMLInputElement>) => void
  labelText: string
  id?: string
  isChecked: boolean
}

function InputCheckbox (props: InputCheckboxProps) {
  return (
    <div className="item">
      <input
        id={props.id}
        type="checkbox"
        checked={props.isChecked}
        onChange={props.onChange}
      />
      <label htmlFor={props.id}>
        {props.labelText}
      </label>
    </div>
  )
}

function HelpImprove () {
  const [isP3AEnabled, setP3AEnabled] = React.useState(false)
  const [isMetricsReportingEnabled, setMetricsReportingEnabled] = React.useState(true)

  const handleP3AChange = () => {
    setP3AEnabled(!isP3AEnabled)
  }

  const handleMetricsReportingChange = () => {
    setMetricsReportingEnabled(!isMetricsReportingEnabled)
  }

  const handleNext = () => {
    BravePrivacyBrowserProxyImpl.getInstance().setP3AEnabled(isP3AEnabled)
    BravePrivacyBrowserProxyImpl.getInstance().setMetricsReportingEnabled(isMetricsReportingEnabled)
    window.open('chrome://newtab', '_self')
  }

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
            id="p3a"
            onChange={handleP3AChange}
            isChecked={isP3AEnabled}
            labelText={getLocale('braveWelcomeSendReportsLabel')}
          />
          <InputCheckbox
            id="metrics"
            onChange={handleMetricsReportingChange}
            isChecked={isMetricsReportingEnabled}
            labelText={getLocale('braveWelcomeSendInsightsLabel')}
          />
        </div>
      </S.Grid>
      <S.ActionBox>
        <div className="box-center">
          <Button
            isPrimary={true}
            onClick={handleNext}
            scale="jumbo"
          >
            {getLocale('braveWelcomeNextButtonLabel')}
          </Button>
          <S.FootNote>
            Change these choices at any time in Brave at brave://settings/privacy.
            {' '}
            Read our full Privacy Policy.
          </S.FootNote>
        </div>
      </S.ActionBox>
    </S.MainBox>
  )
}

export default HelpImprove
