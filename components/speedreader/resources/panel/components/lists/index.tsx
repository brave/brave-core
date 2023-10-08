// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import { FontFamily, FontSize, PlaybackSpeed, PlaybackState, ColumnWidth } from '../../api/browser'
import classnames from '$web-common/classnames'
import { loadTimeData } from '$web-common/loadTimeData'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'

export enum MainButtonType {
  None,
  Tune,
  Appearance,
  TextToSpeech,
  AI
}

const mainButtonsOptions = [
  {
    id: 'tune',
    type: MainButtonType.Tune,
    iconName: 'tune',
    title: getLocale('braveReaderModeTune')
  },
  {
    id: 'appearance',
    type: MainButtonType.Appearance,
    iconName: 'characters',
    title: getLocale('braveReaderModeAppearance')
  },
  {
    id: 'tts',
    type: MainButtonType.TextToSpeech,
    iconName: 'headphones',
    hidden: !loadTimeData.getBoolean('ttsEnabled'),
    title: getLocale('braveReaderModeTextToSpeech')
  },
  {
    id: 'ai',
    type: MainButtonType.AI,
    iconName: 'product-brave-leo',
    hidden: !loadTimeData.getBoolean('aiChatFeatureEnabled'),
    title: getLocale('braveReaderModeAI')
  }
]

const fontStyleOptions = [
  {
    id: 'font-sans',
    family: FontFamily.kSans,
    iconName: 'readermode-sans',
    title: getLocale('braveReaderModeAppearanceFontSans')
  },
  {
    id: 'font-serif',
    family: FontFamily.kSerif,
    iconName: 'readermode-serif',
    title: getLocale('braveReaderModeAppearanceFontSerif')
  },
  {
    id: 'font-mono',
    family: FontFamily.kMono,
    iconName: 'readermode-mono',
    title: getLocale('braveReaderModeAppearanceFontMono')
  },
  {
    id: 'font-dyslexic',
    family: FontFamily.kDyslexic,
    iconName: 'readermode-dislexyc',
    title: getLocale('braveReaderModeAppearanceFontDyslexic')
  }
]

const columnWidthOptions = [
  {
    id: 'column-width-narrow',
    columnWidth: ColumnWidth.kNarrow,
    iconName: 'readermode-column-default',
    title: getLocale('braveReaderModeAppearanceColumnWidthNarrow')
  },
  {
    id: 'column-width-wide',
    columnWidth: ColumnWidth.kWide,
    iconName: 'readermode-column-wide',
    title: getLocale('braveReaderModeAppearanceColumnWidthWide')
  }
]

type OptionType = {
  id?: string
  isSelected: boolean
  children: JSX.Element
  onClick?: Function
  ariaLabel?: string
  inGroup?: boolean
  title?: string
}

function ListBox(props: React.PropsWithChildren<{}>) {
  return (
    <S.Box role="listbox" aria-orientation="horizontal">
      {props.children}
    </S.Box>
  )
}

function ControlButton(props: OptionType) {
  const handleClick = () => {
    props.onClick?.()
  }

  const buttonClass = classnames({
    'is-active': props.isSelected,
  })

  return (
    <S.Button
      id={props?.id}
      role="option"
      className={buttonClass}
      aria-selected={props.isSelected}
      aria-label={props?.ariaLabel}
      onClick={handleClick}
      inGroup={props?.inGroup}
      title={props?.title}
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
      {mainButtonsOptions.filter(entry => { return !entry?.hidden }).map(entry => {
        return (
          <ControlButton
            id={entry.id}
            key={entry.type}
            title={entry.title}
            isSelected={props.activeButton === entry.type}
            onClick={handleClick.bind(this, entry.type)}
          >
            <Icon name={entry.iconName} />
          </ControlButton>
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
          <ControlButton
            id={entry?.id}
            key={entry.family}
            title={entry.title}
            isSelected={props.activeFontFamily === entry.family}
            inGroup={true}
            onClick={handleClick.bind(this, entry.family)}
          >
            <Icon name={entry.iconName} />
          </ControlButton>
        )
      })}
    </ListBox>
  )
}

interface ColumnWidthListProps {
  columnWidth: ColumnWidth
  onClick?: Function
}

export function ColumnWidthList(props: ColumnWidthListProps) {
  const handleClick = (columnWidth: ColumnWidth) => {
    props.onClick?.(columnWidth)
  }
  return (
    <ListBox>
      {columnWidthOptions.map(entry => {
        return (
          <ControlButton
            id={entry?.id}
            key={entry.columnWidth}
            title={entry.title}
            isSelected={props.columnWidth === entry.columnWidth}
            inGroup={true}
            onClick={handleClick.bind(this, entry.columnWidth)}
          >
            <Icon name={entry.iconName} />
          </ControlButton>
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
      <ControlButton
        id='font-size-decrease'
        inGroup={true}
        isSelected={false}
        title={getLocale('braveReaderModeFontSizeDecrease')}
        onClick={() => updateSize(ActionType.Dec)}
      >
        <Icon name='minus' />
      </ControlButton>
      <S.CurrentStateIndicator className='group'>
        <Icon name='font-size' />
        <span>{props.currentSize}% </span>
      </S.CurrentStateIndicator>
      <ControlButton
        id='font-size-increase'
        inGroup={true}
        isSelected={false}
        title={getLocale('braveReaderModeFontSizeIncrease')}
        onClick={() => updateSize(ActionType.Inc)}
      >
        <Icon name='plus-add' />
      </ControlButton>
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
      <ControlButton
        inGroup={true}
        isSelected={false}
        title={getLocale('braveReaderModeTtsRewind')}
        onClick={() => { props.onClick?.(Playback.Rewind) }}
      >
        <Icon name='rewind-outline' />
      </ControlButton>
      <ControlButton
        inGroup={true}
        isSelected={false}
        title={getLocale('braveReaderModeTtsPlayPause')}
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
          {props.playbackState === PlaybackState.kStopped && <Icon name='play-outline' />}
          {props.playbackState === PlaybackState.kPlayingThisPage && <Icon name='pause-outline' />}
          {props.playbackState === PlaybackState.kPlayingAnotherPage && <Icon name='pause-outline' />}
        </div>
      </ControlButton>
      <ControlButton
        inGroup={true}
        isSelected={false}
        title={getLocale('braveReaderModeTtsForward')}
        onClick={() => { props.onClick?.(Playback.Forward) }}
      >
        <Icon name='forward-outline' />
      </ControlButton>
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
      <ControlButton
        inGroup={true}
        isSelected={false}
        title={getLocale('braveReaderModeTtsSpeedDecrease')}
        onClick={() => updateSpeed(ActionType.Dec)}
      >
        <Icon name='minus' />
      </ControlButton>
      <S.CurrentStateIndicator className='group'>
        <Icon name='speed' />
        {props.speed}%
      </S.CurrentStateIndicator>
      <ControlButton
        inGroup={true}
        isSelected={false}
        title={getLocale('braveReaderModeTtsSpeedIncrease')}
        onClick={() => updateSpeed(ActionType.Inc)}
      >
        <Icon name='plus-add' />
      </ControlButton>
    </ListBox>
  )
}
