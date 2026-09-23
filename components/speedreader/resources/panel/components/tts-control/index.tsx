// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'

import getToolbarAPI, { TtsSettings, PlaybackSpeed, PlaybackState } from '../../api/browser'
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
  const [playbackState, setPlaybackState] = React.useState<PlaybackState>(PlaybackState.kStopped)

  React.useEffect(() => {
    const updateVoices = () => {
      setVoices(speechSynthesis.getVoices().filter((v) => {
        return v.default || navigator.languages.find((l) => { return v.lang.startsWith(l) })
      }))
    }
    speechSynthesis.onvoiceschanged = updateVoices
    window.onlanguagechange = updateVoices

    getToolbarAPI().dataHandler.getPlaybackState().then(res => setPlaybackState(res.playbackState))
    getToolbarAPI().eventsRouter.setPlaybackState.addListener((state: PlaybackState) => {
      setPlaybackState(state)
    })
  }, [])

  return (
    <S.Box>
      <PlaybackControl
        playbackState={playbackState}
        onClick={(command: Playback) => {
          switch (command) {
            case Playback.Play:
              getToolbarAPI().dataHandler.play()
              break
            case Playback.Pause:
              getToolbarAPI().dataHandler.pause()
              break
            case Playback.Stop:
              getToolbarAPI().dataHandler.stop()
              break
            case Playback.Forward:
              getToolbarAPI().dataHandler.forward()
              break
            case Playback.Rewind:
              getToolbarAPI().dataHandler.rewind()
              break
          }
        }}
      />
      <PlaybackSpeedControl
        speed={props.ttsSettings.speed}
        onClick={(speed: PlaybackSpeed) => {
          props.onTtsSpeedChange(speed)
        }}
      />
      <S.Voice>
        <span>{getLocale('braveReaderModeVoice')}</span>
        <span>
          <select
            key={props.ttsSettings.voice}
            value={props.ttsSettings.voice}
            onChange={(e) => {
              props.onTtsVoiceChange(e.target.value)
            }}>
            {voices.map(voice => {
              return (
                <option key={voice.name} value={voice.name}>
                  {voice.name}
                </option>)
            })}
          </select>
        </span>
      </S.Voice>
      <S.CloseButton onClick={props.onClose}>
        {getLocale('braveReaderModeClose')}
      </S.CloseButton>
    </S.Box>
  )
}

export default TtsControl
