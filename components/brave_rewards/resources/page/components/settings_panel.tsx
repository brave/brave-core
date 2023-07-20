/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { ToggleButton } from '../../shared/components/toggle_button'
import { TokenAmount } from '../../shared/components/token_amount'
import { ExchangeAmount } from '../../shared/components/exchange_amount'
import { ConfigureIcon } from './icons/configure_icon'
import { CloseIcon } from '../../shared/components/icons/close_icon'

import * as style from './settings_panel.style'

const monthDayFormat = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric'
})

interface PanelProps {
  children: React.ReactNode
  deeplinkId?: string
}

export function SettingsPanel(props: PanelProps) {
  return <style.panel
    data-deeplink-id={props.deeplinkId}>{props.children}</style.panel>
}

interface HeaderProps {
  title: string
  enabled: boolean
  showConfig: boolean
  onEnabledChange?: (enabled: boolean) => void
  onShowConfigChange?: (showConfig: boolean) => void
}

export function PanelHeader (props: HeaderProps) {
  const toggleEnabled = () => {
    if (props.onEnabledChange) {
      props.onEnabledChange(!props.enabled)
    }
  }

  function toggleShowConfig () {
    if (props.onShowConfigChange) {
      props.onShowConfigChange(!props.showConfig)
    }
  }

  function renderConfigToggle () {
    if (!props.onShowConfigChange || !props.enabled) {
      return null
    }

    return (
      <style.config className={props.showConfig ? 'active' : 'inactive'}>
        <button onClick={toggleShowConfig}>
          {props.showConfig ? <CloseIcon /> : <ConfigureIcon />}
        </button>
      </style.config>
    )
  }

  function renderEnableToggle () {
    if (!props.onEnabledChange || props.showConfig) {
      return null
    }

    return (
      <style.toggle data-test-id='setting-enabled-toggle'>
        <ToggleButton checked={props.enabled} onChange={toggleEnabled} />
      </style.toggle>
    )
  }

  return (
    <style.header>
      <style.title>{props.title}</style.title>
      {renderConfigToggle()}
      {renderEnableToggle()}
    </style.header>
  )
}

interface ItemProps {
  label: string
  details?: React.ReactNode
  children: React.ReactNode
}

export function PanelItem (props: ItemProps) {
  return (
    <style.item>
      <style.itemContent>
        <style.itemLabel>{props.label}</style.itemLabel>
        <div>{props.children}</div>
      </style.itemContent>
      {
        props.details &&
          <style.itemDetails>
            {props.details}
          </style.itemDetails>
      }
    </style.item>
  )
}

interface TokenAmountWithExchangeProps {
  amount: number
  exchangeRate: number
  exchangeCurrency: string
}

export function TokenAmountWithExchange (props: TokenAmountWithExchangeProps) {
  return (
    <style.amount>
      <TokenAmount amount={props.amount} minimumFractionDigits={3} />
      <style.exchange>
        <ExchangeAmount
          amount={props.amount}
          rate={props.exchangeRate}
          currency={props.exchangeCurrency}
        />
      </style.exchange>
    </style.amount>
  )
}

interface MonthDayProps {
  date: Date
}

export function MonthDay (props: MonthDayProps) {
  return (
    <style.date>{monthDayFormat.format(props.date)}</style.date>
  )
}

interface TableProps {
  children: React.ReactNode
}

export function PanelTable (props: TableProps) {
  return (
    <style.table>{props.children}</style.table>
  )
}

export function ConfigHeader () {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.configHeader>{getString('settings')}</style.configHeader>
  )
}
