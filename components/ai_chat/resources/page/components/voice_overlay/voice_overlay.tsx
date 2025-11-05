// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styles from './voice_overlay.scss'
import Button from '@brave/leo/react/button'
import { useConversation } from '../../state/conversation_context'
import Icon from '@brave/leo/react/icon'
import { useAIChat } from '../../state/ai_chat_context'
import { VoiceOrb, type OrbState } from './voice_orb'

// Convert AudioBuffer to WAV format
const audioBufferToWav = (audioBuffer: AudioBuffer): Blob => {
  const numberOfChannels = audioBuffer.numberOfChannels
  const sampleRate = audioBuffer.sampleRate
  const format = 1 // PCM
  const bitDepth = 16

  const bytesPerSample = bitDepth / 8
  const blockAlign = numberOfChannels * bytesPerSample

  const data = new Float32Array(audioBuffer.length * numberOfChannels)
  for (let channel = 0; channel < numberOfChannels; channel++) {
    const channelData = audioBuffer.getChannelData(channel)
    for (let i = 0; i < audioBuffer.length; i++) {
      data[i * numberOfChannels + channel] = channelData[i]
    }
  }

  const dataLength = data.length * bytesPerSample
  const buffer = new ArrayBuffer(44 + dataLength)
  const view = new DataView(buffer)

  // WAV header
  const writeString = (offset: number, string: string) => {
    for (let i = 0; i < string.length; i++) {
      view.setUint8(offset + i, string.charCodeAt(i))
    }
  }

  writeString(0, 'RIFF')
  view.setUint32(4, 36 + dataLength, true)
  writeString(8, 'WAVE')
  writeString(12, 'fmt ')
  view.setUint32(16, 16, true) // fmt chunk size
  view.setUint16(20, format, true)
  view.setUint16(22, numberOfChannels, true)
  view.setUint32(24, sampleRate, true)
  view.setUint32(28, sampleRate * blockAlign, true) // byte rate
  view.setUint16(32, blockAlign, true)
  view.setUint16(34, bitDepth, true)
  writeString(36, 'data')
  view.setUint32(40, dataLength, true)

  // PCM data
  let offset = 44
  for (let i = 0; i < data.length; i++) {
    const sample = Math.max(-1, Math.min(1, data[i]))
    const intSample = sample < 0 ? sample * 0x8000 : sample * 0x7FFF
    view.setInt16(offset, intSample, true)
    offset += 2
  }

  return new Blob([buffer], { type: 'audio/wav' })
}

export const VoiceOverlay = () => {
  const conversation = useConversation()
  const service = useAIChat()
  const [isRecording, setIsRecording] = React.useState(false)
  const [error, setError] = React.useState<string | null>(null)
  const [isTranscribing, setIsTranscribing] = React.useState(false)
  const [isTTSLoading, setIsTTSLoading] = React.useState(false)
  const [isAudioPlaying, setIsAudioPlaying] = React.useState(false)
  const [audioLevel, setAudioLevel] = React.useState(0)
  const mediaRecorderRef = React.useRef<MediaRecorder | null>(null)
  const audioChunksRef = React.useRef<Blob[]>([])
  const streamRef = React.useRef<MediaStream | null>(null)
  const audioElementRef = React.useRef<HTMLAudioElement | null>(null)
  const wasGeneratingRef = React.useRef(false)
  const audioContextRef = React.useRef<AudioContext | null>(null)
  const analyserRef = React.useRef<AnalyserNode | null>(null)
  const animationFrameRef = React.useRef<number | null>(null)
  const mediaElementSourceRef = React.useRef<MediaElementAudioSourceNode | null>(null)

  // Monitor when AI finishes generating a response and play it back
  React.useEffect(() => {
    const wasGenerating = wasGeneratingRef.current
    const isGenerating = conversation.isGenerating

    // Detect when generation completes (was generating, now not)
    if (wasGenerating && !isGenerating) {
      // Get the latest assistant message
      const lastMessage = conversation.conversationHistory
        .slice()
        .reverse()
        .find(turn => turn.characterType === 1) // 1 = ASSISTANT

      if (lastMessage?.text) {
        // Call text-to-speech and play the audio
        setIsTTSLoading(true)

        const voiceFriendlyText = lastMessage.text
          .replaceAll(':', '')
          .replaceAll('*', '')
          .replaceAll(/\[[0-9]+\]/g, '')

        service.textToSpeech(voiceFriendlyText).then(audioData => {
          console.log('audioData', audioData)
          setIsTTSLoading(false)
          if (audioData.length > 0) {
            // Create a blob from the audio data (WAV)
            const audioBlob = new Blob([new Uint8Array(audioData)], { type: 'audio/wav' })
            const audioUrl = URL.createObjectURL(audioBlob)

            // Create or reuse audio element
            if (!audioElementRef.current) {
              audioElementRef.current = new Audio()
            }

            audioElementRef.current.src = audioUrl

            // Setup audio analysis for playback (only create source once)
            if (!audioContextRef.current) {
              audioContextRef.current = new AudioContext()
            }
            const audioContext = audioContextRef.current

            // Only create MediaElementSource once per audio element
            if (!mediaElementSourceRef.current) {
              const source = audioContext.createMediaElementSource(audioElementRef.current)
              const analyser = audioContext.createAnalyser()
              analyser.fftSize = 256
              analyser.smoothingTimeConstant = 0.8
              source.connect(analyser)
              analyser.connect(audioContext.destination) // Connect to speakers
              analyserRef.current = analyser
              mediaElementSourceRef.current = source
            }

            // Track audio playback state
            audioElementRef.current.onplay = () => {
              setIsAudioPlaying(true)
              // Start analyzing audio
              if (animationFrameRef.current) {
                cancelAnimationFrame(animationFrameRef.current)
              }
              analyzeAudio()
            }

            audioElementRef.current.onended = () => {
              setIsAudioPlaying(false)
              setAudioLevel(0)
              URL.revokeObjectURL(audioUrl)
              // Stop analyzing audio
              if (animationFrameRef.current) {
                cancelAnimationFrame(animationFrameRef.current)
                animationFrameRef.current = null
              }
            }

            audioElementRef.current.play().catch(err => {
              console.error('Error playing audio:', err)
              setIsAudioPlaying(false)
              setAudioLevel(0)
            })
          }
        }).catch(err => {
          console.error('Error with text-to-speech:', err)
          setIsTTSLoading(false)
        })
      }
    }

    // Update the ref for next comparison
    wasGeneratingRef.current = isGenerating
  }, [conversation.isGenerating, conversation.conversationHistory, service])

  // Cleanup function when component unmounts
  React.useEffect(() => {
    return () => {
      if (mediaRecorderRef.current && mediaRecorderRef.current.state !== 'inactive') {
        mediaRecorderRef.current.stop()
      }
      if (streamRef.current) {
        streamRef.current.getTracks().forEach(track => track.stop())
      }
      if (audioElementRef.current) {
        audioElementRef.current.pause()
        audioElementRef.current.src = ''
      }
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current)
      }
      if (audioContextRef.current) {
        audioContextRef.current.close()
      }
    }
  }, [])

  // Audio analysis loop
  const analyzeAudio = React.useCallback(() => {
    if (!analyserRef.current) return

    const dataArray = new Uint8Array(analyserRef.current.frequencyBinCount)
    analyserRef.current.getByteFrequencyData(dataArray)

    // Calculate average audio level (0-1 range)
    let sum = 0
    for (let i = 0; i < dataArray.length; i++) {
      sum += dataArray[i]
    }
    const average = sum / dataArray.length / 255

    setAudioLevel(average)

    animationFrameRef.current = requestAnimationFrame(analyzeAudio)
  }, [])

  const startListening = async () => {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true })
      streamRef.current = stream

      // Setup audio analysis for recording
      if (!audioContextRef.current) {
        audioContextRef.current = new AudioContext()
      }
      const audioContext = audioContextRef.current
      const source = audioContext.createMediaStreamSource(stream)
      const analyser = audioContext.createAnalyser()
      analyser.fftSize = 256
      analyser.smoothingTimeConstant = 0.8
      source.connect(analyser)
      analyserRef.current = analyser

      // Start analyzing audio
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current)
      }
      analyzeAudio()

      const mediaRecorder = new MediaRecorder(stream)
      mediaRecorderRef.current = mediaRecorder
      audioChunksRef.current = []

      mediaRecorder.ondataavailable = (event) => {
        if (event.data.size > 0) {
          audioChunksRef.current.push(event.data)
        }
      }

      mediaRecorder.start()
      setIsRecording(true)
    } catch (err) {
      console.error('Error accessing microphone:', err)
      setError('Failed to access microphone. Please check permissions.')
    }
  }

  const stopRecording = async () => {
    setIsRecording(false)
    setAudioLevel(0)

    // Stop audio analysis
    if (animationFrameRef.current) {
      cancelAnimationFrame(animationFrameRef.current)
      animationFrameRef.current = null
    }

    if (mediaRecorderRef.current && mediaRecorderRef.current.state !== 'inactive') {
      mediaRecorderRef.current.stop()

      // Wait for the final data to be available
      await new Promise<void>((resolve) => {
        mediaRecorderRef.current!.onstop = () => resolve()
      })
    }

    // Stop all tracks
    if (streamRef.current) {
      streamRef.current.getTracks().forEach(track => track.stop())
    }

    // Convert recorded audio to WAV format
    const audioBlob = new Blob(audioChunksRef.current)
    const arrayBuffer = await audioBlob.arrayBuffer()

    // Decode audio data and convert to WAV
    const audioContext = new AudioContext()
    const audioBuffer = await audioContext.decodeAudioData(arrayBuffer)
    const wavBlob = audioBufferToWav(audioBuffer)

    // Convert WAV to Uint8Array
    const wavArrayBuffer = await wavBlob.arrayBuffer()
    const audioData = new Uint8Array(wavArrayBuffer)

    console.log('audioData', audioData)

    // Send audio data for transcription
    setIsTranscribing(true)
    try {
      const transcription = await service.speechToText(audioData)
      setIsTranscribing(false)
      conversation.submitInputTextToAPI([transcription])
    } catch (err) {
      console.error('Error transcribing audio:', err)
      setIsTranscribing(false)
    }
    // conversation.setVoiceMode(false)
  }

  const handleOrbClick = () => {
    // Don't allow interaction if loading
    if (loadingMessage) {
      return
    }

    if (isRecording) {
      stopRecording()
    } else {
      startListening()
    }
  }

  // Start listening when component mounts
  React.useEffect(() => {
    startListening()
  }, [])

  // Get the most recent message from conversation history
  const lastMessage = conversation.conversationHistory.length > 0
    ? conversation.conversationHistory[conversation.conversationHistory.length - 1]
    : null

  const isUserMessage = lastMessage?.characterType === 0 // 0 = HUMAN

  // Determine loading state and message
  const getLoadingState = () => {
    if (isTranscribing) return 'Transcribing...'
    if (conversation.isGenerating) return 'AI is thinking...'
    if (isTTSLoading) return 'Generating speech...'
    if (isAudioPlaying) return 'Playing...'
    return null
  }

  const loadingMessage = getLoadingState()

  // Determine orb state
  const getOrbState = (): OrbState => {
    if (isRecording) return 'listening'
    if (isTranscribing || conversation.isGenerating || isTTSLoading) return 'thinking'
    if (isAudioPlaying) return 'speaking'
    return 'idle'
  }

  const orbState = getOrbState()

  return <div className={styles.voiceOverlay} >
    <div
      className={`${styles.orb} ${isRecording ? styles.recording : ''} ${loadingMessage ? styles.loading : ''}`}
      onClick={handleOrbClick}
    >
      <VoiceOrb state={orbState} audioLevel={audioLevel} />
    </div>
    {loadingMessage && (
      <div className={styles.loadingContainer}>
        <div className={styles.loadingSpinner}>
          <div className={styles.spinner}></div>
        </div>
        <div className={styles.loadingText}>{loadingMessage}</div>
      </div>
    )}
    {error && <div className={styles.error}>{error}</div>}
    {isRecording && <div className={styles.recordingIndicator}>Recording...</div>}
    {lastMessage && (
      <div className={styles.messageDisplay}>
        <div className={styles.messageLabel}>
          {isUserMessage ? 'You' : 'AI'}
        </div>
        <div className={styles.messageText}>
          {lastMessage.text || '...'}
        </div>
      </div>
    )}
    <Button className={styles.closeButton} fab kind='plain' onClick={() => conversation.setVoiceMode(false)}>
      <Icon name='close' />
    </Button>
  </div>
}
