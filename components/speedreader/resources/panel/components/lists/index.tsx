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
import PauseSVG from '../../svg/pause_outline'
import ForwardSVG from '../../svg/forward_outline'
import SpeedSVG from '../../svg/speed'
import FontSizeSVG from '../../svg/font_size'
import { FontFamily, FontSize, PlaybackSpeed, PlaybackState } from '../../api/browser'
import classnames from '$web-common/classnames'

export enum MainButtonType {
  None,
  Options,
  TextToSpeech,
  ViewOriginal,
  AI
}

const mainButtonsOptions = [
  {
    id: 'options',
    type: MainButtonType.Options,
    svgIcon: SettingsSVG
  },
  {
    id: 'tts',
    type: MainButtonType.TextToSpeech,
    svgIcon: HeadphonesSVG
  },
  {
    id: 'view-original',
    type: MainButtonType.ViewOriginal,
    svgIcon: OriginalSVG
  },
  {
    id: 'ai',
    type: MainButtonType.AI,
    svgIcon: AiSVG
  }
]

const fontStyleOptions = [
  {
    id: 'font-sans',
    family: FontFamily.kSans,
    svgIcon: fontSansSvg
  },
  {
    id: 'font-serif',
    family: FontFamily.kSerif,
    svgIcon: fontSerifSvg
  },
  {
    id: 'font-mono',
    family: FontFamily.kMono,
    svgIcon: fontMonoSvg
  },
  {
    id: 'font-dyslexic',
    family: FontFamily.kDyslexic,
    svgIcon: fontDyslexicSvg
  }
]

type OptionType = {
  id?: string
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
      id={props?.id}
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
            id={entry.id}
            key={entry.type}
            isSelected={props.activeButton === entry.type}
            onClick={handleClick.bind(this, entry.type)}
          >
            {<entry.svgIcon />}
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
            id={entry?.id}
            key={entry.family}
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
    const newSize = action === ActionType.Dec ?
      props.currentSize - 10 : props.currentSize + 10

    if (newSize >= FontSize.MIN_VALUE && newSize <= FontSize.MAX_VALUE) {
      props.onClick?.(newSize)
      return
    }
    return props.onClick?.(props.currentSize)
  }

  return (
    <ListBox>
      <Option
        id='font-size-decrease'
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
        id='font-size-increase'
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

interface PlaybackControlProps {
  playbackState: PlaybackState
  onClick?: Function
}

export function PlaybackControl(props: PlaybackControlProps) {
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
        onClick={() => {
          switch (props.playbackState) {
            case PlaybackState.kPlayingThisPage:
              props.onClick?.(Playback.Pause)
              break
            case PlaybackState.kPlayingAnotherPage:
              props.onClick?.(Playback.Stop)
              break
            case PlaybackState.kStopped:
              props.onClick?.(Playback.Play)
              break
          }
        }}
      >
        <div>
          {props.playbackState === PlaybackState.kStopped && <PlaySVG />}
          {props.playbackState === PlaybackState.kPlayingThisPage && <PauseSVG />}
          {props.playbackState === PlaybackState.kPlayingAnotherPage && <PauseSVG />}
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

interface PlaybackSpeedControlProps {
  speed: PlaybackSpeed
  onClick?: Function
}

export function PlaybackSpeedControl(props: PlaybackSpeedControlProps) {
  enum ActionType {
    Inc,
    Dec
  }

  const updateSpeed = (action: ActionType) => {
    const newSpeed = action === ActionType.Dec ?
      props.speed - 10 : props.speed + 10
    if (newSpeed >= PlaybackSpeed.MIN_VALUE &&
        newSpeed <= PlaybackSpeed.MAX_VALUE) {
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