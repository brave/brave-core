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
import contentTextOnlySvg from '../../svg/contentTextOnly'
import contentTextWithImagesSvg from '../../svg/contentTextWithImages'
import { FontFamily, ContentStyle } from '../../api/browser'

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

const contentStyleOptions = [
  {
    title: 'Text with images',
    contentStyle: ContentStyle.kDefault,
    svgIcon: contentTextWithImagesSvg
  },
  {
    title: 'Text only',
    contentStyle: ContentStyle.kTextOnly,
    svgIcon: contentTextOnlySvg
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
              {entry.title}
            </div>
          </Option>
        )
      })}
    </ListBox>
  )
}

interface ContentStyleProps {
  activeContentStyle: ContentStyle
  onClick?: Function
}

export function ContentList (props: ContentStyleProps) {
  const handleClick = (contentStyle: ContentStyle) => {
    props.onClick?.(contentStyle)
  }

  return (
    <ListBox>
      {contentStyleOptions.map(entry => {
        return (
          <Option
            key={entry.title}
            isSelected={props.activeContentStyle === entry.contentStyle}
            ariaLabel={entry.title}
            onClick={handleClick.bind(this, entry.contentStyle)}
          >
            <div>{<entry.svgIcon />}</div>
          </Option>
        )
      })}
    </ListBox>
  )
}
