// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import classnames from '$web-common/classnames'
import CheckMarkSvg from '../../svg/check_circle_filled'
import ThemeSystemSvg from '../../svg/dark_mode'
import { Theme } from '../../api/browser'

const themeOptions = [
  {
    id: 'theme-light',
    type: Theme.kLight,
    svgIcon: null
  },
  {
    id: 'theme-sepia',
    type: Theme.kSepia,
    svgIcon: null
  },
  {
    id: 'theme-dark',
    type: Theme.kDark,
    svgIcon: null
  },
  {
    id: 'theme-system',
    type: Theme.kNone,
    svgIcon: ThemeSystemSvg
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
        const chipClass = classnames({
          'chip': true,
          'is-light': entry.type === Theme.kLight,
          'is-dark': entry.type === Theme.kDark,
          'is-sepia': entry.type === Theme.kSepia
        })
        return (
          <button
            id={entry.id}
            key={entry.type}
            role="option"
            className={chipClass}
            aria-selected={props.activeTheme === entry.type}
            onClick={handleClick.bind(this, entry.type)}
          >
            {entry.svgIcon && (
              <entry.svgIcon className='icon-box' />
            )}
            {props.activeTheme === entry.type && (
              <CheckMarkSvg className='mark' id='check-mark' />
            )}
          </button>
        )
      })}
    </S.Box>
  )
}

export default ThemeControl
