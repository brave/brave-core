// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation
import Speech
import SwiftUI
import os.log

protocol SpeechRecognizerDelegate: AnyObject {
  func speechRecognizerDidFinishQuery(query: String)
}

class SpeechRecognizer: ObservableObject {
  
  private let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "voiceRecognizer")
  
  enum RecognizerError: Error {
    case microphoneAccessDenied
    case authorizationAccessDenied
  }
  
  enum AnimationType {
    case speech(volume: CGFloat)
    case pulse(scale: CGFloat)
    case stable
  }
  
  private struct AnimationScale {
    static let max = 1.50
    static let pulse = 0.75
  }
  
  weak var delegate: SpeechRecognizerDelegate?
  
  /// Formatted transcript from speech recognizer
  @Published var transcript: String = " "
  @Published var transcriptedIcon: String = "leo.microphone"
  @Published var finalizedRecognition: (status: Bool, searchQuery: String) = (false, "")
  @Published private(set) var animationType: AnimationType = .pulse(scale: 1)

  var isVoiceSearchAvailable: Bool {
    if let recognizer, recognizer.isAvailable, recognizer.supportsOnDeviceRecognition {
      return true
    }
    
    return false
  }
  
  private var isSilent = true
    
  private var audioEngine: AVAudioEngine?
  private var request: SFSpeechAudioBufferRecognitionRequest?
  private var task: SFSpeechRecognitionTask?
  private let recognizer = SFSpeechRecognizer()
  
  @MainActor
  func askForUserPermission() async -> Bool {
    // Ask for Record Permission if not permitted throw error
    guard await AVAudioSession.sharedInstance().hasPermissionToRecord() else {
      return false
    }
    
    return true
  }
  
  @MainActor
  func startTranscribing() {
    transcribe()
  }
    
  @MainActor
  func stopTranscribing() {
    reset()
  }
  
  func startSilenceAnimation() {
    animationType = .pulse(scale: AnimationScale.pulse)
  }

  func startSpeechAnimation(_ scale: CGFloat) {
    animationType = .speech(volume: scale)
  }
  
  /// Creates a `SFSpeechRecognitionTask` that transcribes speech to text until you call `stopTranscribing()`.
  /// The resulting transcription is continuously written to the published `transcript` property.
  private func transcribe() {
    guard let recognizer, recognizer.isAvailable else {
      log.debug("Voice Search Unavailable: ")
      return
    }
    
    do {
      let (audioEngine, request) = try setupStartEngine()
        
      self.audioEngine = audioEngine
      self.request = request
      
      task = recognizer.recognitionTask(with: request, resultHandler: { [weak self] result, error in
        guard let self = self else {
          return
        }
        
        var isFinal = false
        
        if let result, !isFinal {
          let formattedTranscript = result.bestTranscription.formattedString
          let transcriptComponents = formattedTranscript.components(separatedBy: .whitespacesAndNewlines)
          let formattedWords = transcriptComponents.filter { !$0.isEmpty }
          
          // SpeechRecognitionMetadata is the key to detect speaking finalized
          isFinal = result.isFinal
          || result.speechRecognitionMetadata != nil
          || formattedWords.count >= 15
              
          if !formattedTranscript.isEmpty {
            self.transcribe(formattedTranscript)
          }
        }
        
        // Check voice input final
        if isFinal {
          animationType = .stable
          transcriptedIcon = "leo.check.circle-outline"
          
          // Remove audio buffer input
          audioEngine.inputNode.removeTap(onBus: 0)
          // Reset Speech Recognizer
          self.reset()

          Task.delayed(bySeconds: 0.75) { @MainActor in
            if let formattedTranscript = result?.bestTranscription.formattedString, !formattedTranscript.isEmpty {
              self.finalize(searchQuery: formattedTranscript)
            }
          }
        }
      })
    } catch {
      reset()
      log.debug("Voice Search Recognization Fault \(error.localizedDescription)")
    }
  }
    
  /// Reset the speech recognizer.
  private func reset() {
    try? AVAudioSession.sharedInstance().setActive(false)
    task?.cancel()
    audioEngine?.stop()
    
    audioEngine = nil
    request = nil
    task = nil
  }
    
  private func setupStartEngine() throws -> (AVAudioEngine, SFSpeechAudioBufferRecognitionRequest) {
    let audioEngine = AVAudioEngine()
    
    let request = SFSpeechAudioBufferRecognitionRequest()
    request.shouldReportPartialResults = true
    request.requiresOnDeviceRecognition = true

    let audioSession = AVAudioSession.sharedInstance()
    try audioSession.setCategory(.playAndRecord, mode: .measurement, options: .duckOthers)
    try audioSession.setActive(true, options: .notifyOthersOnDeactivation)
    let inputNode = audioEngine.inputNode
    
    let recordingFormat = inputNode.outputFormat(forBus: 0)
    inputNode.installTap(onBus: 0, bufferSize: 1024, format: recordingFormat) { [weak self] buffer, when in
      guard let self else { return }
      
      request.append(buffer)
      
      guard let channelData = buffer.floatChannelData?[0] else {
        return
      }
      
      let volume = self.getVolumeLevel(from: channelData)
      
      Task { @MainActor in
        self.setupAnimationWithVolume(volume)
      }
    }
    
    audioEngine.prepare()
    try audioEngine.start()
    
    return (audioEngine, request)
  }

  nonisolated private func transcribe(_ message: String) {
    Task { @MainActor in
      transcript = message
    }
  }
  
  nonisolated private func finalize(searchQuery: String) {
    Task { @MainActor in
      if !finalizedRecognition.status {
        finalizedRecognition = (true, searchQuery)
      }
    }
  }
  
  nonisolated func clearSearch() {
    Task { @MainActor in
      transcript = " "
      transcriptedIcon = "leo.microphone"
      finalizedRecognition = (false, "")
      animationType = .pulse(scale: 1)
      isSilent = true
    }
  }
  
  private func getVolumeLevel(from channelData: UnsafeMutablePointer<Float>) -> Float {
    let channelDataArray = Array(UnsafeBufferPointer(start: channelData, count: 1024))

    // This method takes incoming sample between -1 and +1 makes it absolute
    // If current rectified sample is larger than the 'envelopeState
    // the envelopeState is increased by 0.16 of the difference
    // If it is smaller the envelopeState is decreased by 0.003 of the difference
    // 'envelopeState' will rapidly rise if there is input, and slowly fall if there is none
    var outEnvelope = [Float]()
    var envelopeState: Float = 0
    let envConstantAtk: Float = 0.16
    let envConstantDec: Float = 0.003

    for sample in channelDataArray {
      let rectified = abs(sample)

      if envelopeState < rectified {
        envelopeState += envConstantAtk * (rectified - envelopeState)
      } else {
        envelopeState += envConstantDec * (rectified - envelopeState)
      }
      outEnvelope.append(envelopeState)
    }

    // The buffer is considered full of background noise only if that peak value is less than 0.015
    // 0.015 is the low pass filter to prevent getting the noise entering from the microphone
    if let maxVolume = outEnvelope.max(), maxVolume > Float(0.015) {
      return maxVolume
    } else {
      return 0.0
    }
  }
  
  private func setupAnimationWithVolume(_ volume: Float) {
    let isCurrentlySilent = volume <= 0
    // We want to make sure that every detected sound makes the outer circle bigger
    let minScale: CGFloat = 1.25
    
    if !isCurrentlySilent {
      let scaleValue = min(CGFloat(volume) + minScale, AnimationScale.max)
      startSpeechAnimation(scaleValue)
    }
    
    if !isSilent && isCurrentlySilent {
      startSilenceAnimation()
    }
    
    isSilent = isCurrentlySilent
  }
}

extension AVAudioSession {
  /// Ask for recording permission
  ///  this is used for access microphone
  /// - Returns: Authorization state
  func hasPermissionToRecord() async -> Bool {
    await withCheckedContinuation { continuation in
      requestRecordPermission { authorized in
        continuation.resume(returning: authorized)
      }
    }
  }
}
