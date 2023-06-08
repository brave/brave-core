// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'

import { TtsSettings, dataHandler, PlaybackSpeed, eventsHandler, PlaybackState } from '../../api/browser'
import { Playback, PlaybackControl, PlaybackSpeedControl } from '../lists'
import { getLocale } from '$web-common/locale'

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

  const [playbackState, setPlaybackState] = React.useState<PlaybackState>(PlaybackState.kStopped)
  dataHandler.getPlaybackState().then(res => setPlaybackState(res.playbackState))
  eventsHandler.setPlaybackState.addListener((state: PlaybackState) => {
    setPlaybackState(state)
  })

  return (
    <S.Box>
      <PlaybackControl
        playbackState={playbackState}
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
      <PlaybackSpeedControl
        speed={props.ttsSettings.speed}
        onClick={(speed: PlaybackSpeed) => {
          props.onTtsSpeedChange(speed)
        }}
      />
      <S.VDelemiter />
      <S.Voice>
        <span>{getLocale('braveReaderModeVoice')}</span>
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
      <S.Close onClick={props.onClose}>
        {getLocale('braveReaderModeClose')}
      </S.Close>
    </S.Box>
  )
}

export default TtsControl
