// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import classnames from '$web-common/classnames'
import themeLightSvg from '../../svg/themeLight'
import themeDarkSvg from '../../svg/themeDark'
import themeSepiaSvg from '../../svg/themeSepia'
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
  }
]

interface ThemeControlProps {
  activeTheme: Theme
  onClick?: Function
}

function ThemeControl (props: ThemeControlProps) {
  const handleClick = (themeType: Theme) => {
    props.onClick?.(themeType)
  }

  return (
    <S.Box role="listbox">
      {themeOptions.map(entry => {
        const chipClass = classnames({
          'chip': true,
          'is-active': props.activeTheme === entry.type
        })

        const iconClass = classnames({
          'icon-box': true,
          'is-light': entry.type === Theme.kLight,
          'is-dark': entry.type === Theme.kDark,
          'is-sepia': entry.type === Theme.kSepia
        })

        return (
          <button
            key={entry.type}
            role="option"
            className={chipClass}
            aria-selected={props.activeTheme === entry.type}
            aria-label={entry.ariaLabel}
            onClick={handleClick.bind(this, entry.type)}
          >
            <div className={iconClass}>
              {<entry.svgIcon />}
            </div>
            {props.activeTheme === entry.type && (
              <i><svg name="checkmark-icon" width="18" height="18" fill="none" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M9 17.333C4.405 17.333.667 13.595.667 9 .667 4.405 4.405.667 9 .667c4.595 0 8.333 3.738 8.333 8.333 0 4.595-3.738 8.333-8.333 8.333Zm-.172-5.098a.691.691 0 0 1-.956.085L4.4 9.542a.695.695 0 0 1 .867-1.084L8.22 10.82l4.424-5.055a.695.695 0 1 1 1.045.914l-4.861 5.556Z" fill="#737ADE"/></svg></i>
            )}
          </button>
        )
      })}
    </S.Box>
  )
}

export default ThemeControl
