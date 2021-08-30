/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'

import { SettingsIcon } from './icons/settings_icon'
import { SummaryIcon } from './icons/summary_icon'
import { TipIcon } from './icons/tip_icon'

import * as style from './navbar.style'

type ActiveView = 'tip' | 'summary'

interface Props {
  canTip: boolean
  activeView: ActiveView
  onActiveViewChange: (activeView: ActiveView) => void
  onSettingsClick: () => void
}

export function NavBar (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function clickHandler (view: ActiveView) {
    return () => {
      if (view !== props.activeView) {
        props.onActiveViewChange(view)
      }
    }
  }

  function selectedClass (view: ActiveView) {
    return props.activeView === view ? 'selected' : ''
  }

  return (
    <style.root>
      <style.tip className={selectedClass('tip')}>
        <button onClick={clickHandler('tip')} disabled={!props.canTip}>
          <TipIcon /> {getString('tip')}
        </button>
      </style.tip>
      <style.summary className={selectedClass('summary')}>
        <button onClick={clickHandler('summary')}>
          <SummaryIcon /> {getString('summary')}
        </button>
      </style.summary>
      <style.settings>
        <button onClick={props.onSettingsClick}>
          <SettingsIcon />
        </button>
      </style.settings>
    </style.root>
  )
}
