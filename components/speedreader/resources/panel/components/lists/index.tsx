// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import fontSerifSvg from '../../svg/fontSerif'
import fontSansSvg from '../../svg/fontSans'
import fontMonoSvg from '../../svg/fontMono'
import fontDyslexicSvg from '../../svg/fontDyslexic'
import SettingsSVG from '../../svg/settings'
import HeadphonesSVG from '../../svg/headphones'
import OriginalSVG from '../../svg/original'
import AiSVG from '../../svg/ai'
import { FontFamily } from '../../api/browser'

export enum MainButtonType {
  None,
  Options,
  TextToSpeech,
  ShowOriginal,
  AI
}

const mainButtonsOptions = [
  {
    type: MainButtonType.Options,
    svgIcon: SettingsSVG
  },
  {
    type: MainButtonType.TextToSpeech,
    svgIcon: HeadphonesSVG
  },
  {
    type: MainButtonType.ShowOriginal,
    svgIcon: OriginalSVG
  },
  {
    type: MainButtonType.AI,
    svgIcon: AiSVG
  }
]

const fontStyleOptions = [
  {
    title: 'Sans',
    family: FontFamily.kSans,
    svgIcon: fontSansSvg
  },
  {
    title: 'Serif',
    family: FontFamily.kSerif,
    svgIcon: fontSerifSvg
  },
  {
    title: 'Mono',
    family: FontFamily.kMono,
    svgIcon: fontMonoSvg
  },
  {
    title: 'Dyslexic',
    family: FontFamily.kDyslexic,
    svgIcon: fontDyslexicSvg
  }
]

type OptionType = {
  isSelected: boolean
  children: JSX.Element
  onClick?: Function
  ariaLabel?: string
}

function ListBox (props: React.PropsWithChildren<{}>) {
  return (
    <S.Box role="listbox" aria-orientation="horizontal">
      {props.children}
    </S.Box>
  )
}

function Option (props: OptionType) {
  const handleClick = () => {
    props.onClick?.()
  }

  return (
    <button
      role="option"
      className={props.isSelected ? 'is-active' : ''}
      aria-selected={props.isSelected}
      aria-label={props?.ariaLabel}
      onClick={handleClick}
    >
      {props.children}
    </button>
  )
}

interface MainButtonsListProps {
  activeButton: MainButtonType;
  onClick?: Function
}

export function MainButtonsList (props: MainButtonsListProps) {
  const handleClick = (active: MainButtonType) => {
    props.onClick?.(active)
  }

  return (
    <ListBox>
      {mainButtonsOptions.map(entry => {
        return (
          <Option
            key={entry.type}
            isSelected={props.activeButton === entry.type}
            onClick={handleClick.bind(this, entry.type)}
          >
           <div>{<entry.svgIcon />}</div>
          </Option>
        )
      })}        
    </ListBox>
  )
}

interface FontStyleListProps {
  activeFontFamily: FontFamily
  onClick?: Function
}

export function FontStyleList (props: FontStyleListProps) {
  const handleClick = (fontFamily: FontFamily) => {
    props.onClick?.(fontFamily)
  }

  return (
    <ListBox>
      {fontStyleOptions.map(entry => {
        return (
          <Option
            key={entry.title}
            isSelected={props.activeFontFamily === entry.family}
            onClick={handleClick.bind(this, entry.family)}
          >
            <div className="sm">
              <div>{<entry.svgIcon />}</div>
            </div>
          </Option>
        )
      })}
    </ListBox>
  )
}
