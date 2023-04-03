// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import CheckMarkSvg from '../../svg/checkMark'
import themeLightSvg from '../../svg/themeLight'
import themeDarkSvg from '../../svg/themeDark'
import themeSepiaSvg from '../../svg/themeSepia'
import themeSystemSvg from '../../svg/themeSystem'
import { Theme } from '../../api/browser'

const themeOptions = [
  {
    ariaLabel: 'Theme Light',
    type: Theme.kLight,
    svgIcon: themeLightSvg
  },
  {
    ariaLabel: 'Theme Sepia',
    type: Theme.kSepia,
    svgIcon: themeSepiaSvg
  },
  {
    ariaLabel: 'Theme Dark',
    type: Theme.kDark,
    svgIcon: themeDarkSvg
  },
  {
    ariaLable: 'Theme System',
    type: Theme.kNone,
    svgIcon: themeSystemSvg
  }
]

interface ThemeControlProps {
  activeTheme: Theme
  onClick?: Function
}

function ThemeControl(props: ThemeControlProps) {
  const handleClick = (themeType: Theme) => {
    props.onClick?.(themeType)
  }

  return (
    <S.Box role="listbox">
      {themeOptions.map(entry => {
        return (
          <button
            key={entry.type}
            role="option"
            className='chip'
            aria-selected={props.activeTheme === entry.type}
            aria-label={entry.ariaLabel}
            onClick={handleClick.bind(this, entry.type)}
          >
            <div className='icon-box'>
              {<entry.svgIcon />}
            </div>
            {props.activeTheme === entry.type && (
              <CheckMarkSvg />
            )}
          </button>
        )
      })}
    </S.Box>
  )
}

export default ThemeControl
