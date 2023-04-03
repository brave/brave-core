// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'

import { TtsSettings, dataHandler, PlaybackSpeed, eventsHandler, ReadingState } from '../../api/browser'
import { Playback, PlaybackList, PlaybackSpeedList } from '../lists'

interface TtsControlProps {
  ttsSettings: TtsSettings
  onTtsVoiceChange: (voice: string) => void;
  onTtsSpeedChange: (speed: PlaybackSpeed) => void
  onClose: () => void
}

function TtsControl(props: TtsControlProps) {
  const [voices, setVoices] = React.useState(speechSynthesis.getVoices())
  speechSynthesis.onvoiceschanged = () => {
    setVoices(speechSynthesis.getVoices())
  }

  const [isPlaying, setPlaying] = React.useState(false)
  eventsHandler.onReadingStateChanged.addListener((state: ReadingState) => {
    setPlaying(state === ReadingState.kPlaying)
  })

  return (
    <S.Box>
      <PlaybackList
        isPlaying={isPlaying}
        onClick={(command: Playback) => {
          switch (command) {
            case Playback.Play:
              dataHandler.play()
              break
            case Playback.Pause:
              dataHandler.pause()
              break
            case Playback.Stop:
              dataHandler.stop()
              break              
            case Playback.Forward:
              dataHandler.forward()
              break
            case Playback.Rewind:
              dataHandler.rewind()
              break
          }
        }}
      />
      <S.VDelemiter />
      <PlaybackSpeedList
        speed={props.ttsSettings.speed}
        onClick={(speed: PlaybackSpeed) => {
          props.onTtsSpeedChange(speed)
        }}
      />
      <S.VDelemiter />
      <S.Voice>
        <span>Voice</span>
        <span>
          <select
            value={props.ttsSettings.voice}
            onChange={(e) => {
              props.onTtsVoiceChange(e.target.value)
            }}>
            {voices.map(voice => {
              return (
                <option value={voice.name}>
                  {voice.name}
                </option>)
            })}
          </select>
        </span>
      </S.Voice>
      <S.VDelemiter />
      <S.Close onClick={props.onClose}>Close</S.Close>
    </S.Box>
  )
}

export default TtsControl
