// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import fontSerifSvg from '../../svg/readermode_serif'
import fontSansSvg from '../../svg/readermode_sans'
import fontMonoSvg from '../../svg/readermode_mono'
import fontDyslexicSvg from '../../svg/readermode_dislexyc'
import SettingsSVG from '../../svg/characters'
import HeadphonesSVG from '../../svg/headphones'
import OriginalSVG from '../../svg/product_speedreader'
import AiSVG from '../../svg/product_brave_ai'
import PlusSVG from '../../svg/plus_add'
import MinusSVG from '../../svg/minus'
import RewindSVG from '../../svg/rewind_outline'
import PlaySVG from '../../svg/play_outline'
import StopSVG from '../../svg/pause_outline'
import ForwardSVG from '../../svg/forward_outline'
import SpeedSVG from '../../svg/speed'
import FontSizeSVG from '../../svg/font_size'
import { FontFamily, FontSize, PlaybackSpeed } from '../../api/browser'
import classnames from '$web-common/classnames'

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
  inGroup?: boolean
}

function ListBox(props: React.PropsWithChildren<{}>) {
  return (
    <S.Box role="listbox" aria-orientation="horizontal">
      {props.children}
    </S.Box>
  )
}

function Option(props: OptionType) {
  const handleClick = () => {
    props.onClick?.()
  }

  const optionClass = classnames({
    'is-active': props.isSelected,
    'group': props?.inGroup
  })

  return (
    <S.Button
      role="option"
      className={optionClass}
      aria-selected={props.isSelected}
      aria-label={props?.ariaLabel}
      onClick={handleClick}
    >
      {props.children}
    </S.Button>
  )
}

interface MainButtonsListProps {
  activeButton: MainButtonType;
  onClick?: Function
}

export function MainButtonsList(props: MainButtonsListProps) {
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
            {<entry.svgIcon/>}
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

export function FontStyleList(props: FontStyleListProps) {
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
            inGroup={true}
            onClick={handleClick.bind(this, entry.family)}
          >
            {<entry.svgIcon />}
          </Option>
        )
      })}
    </ListBox>
  )
}

interface FontSizeListProps {
  currentSize: FontSize
  onClick?: Function
}

export function FontSizeList(props: FontSizeListProps) {
  enum ActionType {
    Inc,
    Dec
  }

  const updateSize = (action: ActionType) => {
    const newSize = action === ActionType.Dec ? props.currentSize - 10 : props.currentSize + 10
    if (newSize >= FontSize.MIN_VALUE && newSize <= FontSize.MAX_VALUE) {
      props.onClick?.(newSize)
      return
    }
    return props.onClick?.(props.currentSize)
  }

  return (
    <ListBox>
      <Option
        inGroup={true}
        isSelected={false}
        onClick={() => updateSize(ActionType.Dec)}
      >
        <MinusSVG />
      </Option>
      <S.CurrentState className='group' disabled={true}>
        <FontSizeSVG />
        <span>{props.currentSize}% </span>
      </S.CurrentState>
      <Option
        inGroup={true}
        isSelected={false}
        onClick={() => updateSize(ActionType.Inc)}
      >
        <PlusSVG />
      </Option>
    </ListBox>
  )
}

export enum Playback {
  Rewind,
  Play,
  Pause,
  Stop,
  Forward
}

interface PlaybackListProps {
  isPlaying: boolean
  onClick?: Function
}

export function PlaybackList(props: PlaybackListProps) {
  return (
    <ListBox>
      <Option
        inGroup={true}
        isSelected={false}
        onClick={() => { props.onClick?.(Playback.Rewind) }}
      >
        <RewindSVG />
      </Option>
      <Option
        inGroup={true}
        isSelected={false}        
        onClick={() => { props.onClick?.(props.isPlaying ? Playback.Stop : Playback.Play) }}
      >
        <div>
          {props.isPlaying ? <StopSVG /> : <PlaySVG />}
        </div>
      </Option>
      <Option
        inGroup={true}
        isSelected={false}
        onClick={() => { props.onClick?.(Playback.Forward) }}
      >
        <ForwardSVG />
      </Option>
    </ListBox>
  )
}

interface PlaybackSpeedListProps {
  speed: PlaybackSpeed
  onClick?: Function
}

export function PlaybackSpeedList(props: PlaybackSpeedListProps) {
  enum ActionType {
    Inc,
    Dec
  }
  
  const updateSpeed = (action: ActionType) => {
    const newSpeed = action === ActionType.Dec ? props.speed - 10 : props.speed + 10
    if (newSpeed >= PlaybackSpeed.MIN_VALUE && newSpeed <= PlaybackSpeed.MAX_VALUE) {
      props.onClick?.(newSpeed)
      return
    }
    return props.onClick?.(props.speed)
  }

  return (
    <ListBox>
      <Option
        inGroup={true}
        isSelected={false}
        onClick={() => updateSpeed(ActionType.Dec)}
      >
        <MinusSVG />
      </Option>
      <S.CurrentState className='group' disabled={true}>
        <SpeedSVG />
        {props.speed}%
      </S.CurrentState>
      <Option
        inGroup={true}
        isSelected={false}
        onClick={() => updateSpeed(ActionType.Inc)}
      >
        <PlusSVG />
      </Option>
    </ListBox>
  )
}