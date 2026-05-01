// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  sendTokenizedWebKitMessageSynchronously,
  messageHandlerName,
} from '//brave/ios/web/js_messaging/resources/utils.js'

interface MimeTypeData {
  suffixes: string
  type: string
  description: string
}

interface PluginData {
  description: string
  name: string
  filename: string
  mimeTypes: MimeTypeData[]
}

interface FarblingArgs {
  fudgeFactor: number
  fakePluginData: PluginData[]
  fakeVoiceName: string
  randomVoiceIndexScale: number
  randomHardwareIndexScale: number
}

interface Message {
  isFarblingEnabled: boolean
  args: FarblingArgs | null
}

const message: Message = sendTokenizedWebKitMessageSynchronously(
  messageHandlerName,
  {},
)
if (message?.isFarblingEnabled && message.args) {
  enableFarbling(message.args)
}

function enableFarbling(args: FarblingArgs) {
  // 1. Farble audio
  // Adds slight randization when reading data for audio files
  // Randomization is determined by the fudge factor
  const farbleAudio = (fudgeFactor: number) => {
    if (window.AudioBuffer === undefined) {
      return
    }

    const farbleArrayData = (destination: Float32Array | Uint8Array) => {
      // Let's fudge the data by our fudge factor.
      destination.forEach((value, index) => {
        destination[index] = value * fudgeFactor
      })
    }

    // Farble "destination" methods
    const structuresToFarble: [typeof AnalyserNode, keyof AnalyserNode][] = [
      [window.AnalyserNode, 'getFloatFrequencyData'],
      [window.AnalyserNode, 'getByteFrequencyData'],
      [window.AnalyserNode, 'getByteTimeDomainData'],
      [window.AnalyserNode, 'getFloatTimeDomainData'],
    ]

    for (const [structure, methodName] of structuresToFarble) {
      const proto = structure.prototype as unknown as Record<string, Function>
      const origImplementation = proto[methodName as string]!
      proto[methodName as string] = function (...args: unknown[]) {
        Reflect.apply(origImplementation, this, args)
        farbleArrayData(args[0] as Float32Array | Uint8Array)
      }
    }
  }

  // 2. Farble plugin data
  // Injects fake plugins with fake mime-types
  // Random plugins are determined by the plugin data
  const farblePlugins = (fakePluginData: PluginData[]) => {
    if (fakePluginData.length === 0) {
      return
    }
    // Function that create a fake mime-type based on the given fake data
    const makeFakeMimeType = (fakeData: MimeTypeData): MimeType => {
      return Object.create(window.MimeType.prototype, {
        suffixes: { value: fakeData.suffixes },
        type: { value: fakeData.type },
        description: { value: fakeData.description },
      })
    }

    // Create a fake plugin given the plugin data
    const makeFakePlugin = (pluginData: PluginData): Plugin => {
      const newPlugin: Plugin & Record<string | number, unknown> =
        Object.create(window.Plugin.prototype, {
          description: { value: pluginData.description },
          name: { value: pluginData.name },
          filename: { value: pluginData.filename },
          length: { value: pluginData.mimeTypes.length },
        })

      // Create mime-types and link them to the new plugin
      for (const [index, mimeType] of pluginData.mimeTypes.entries()) {
        const newMimeType = makeFakeMimeType(mimeType)

        newPlugin[index] = newMimeType
        newPlugin[newMimeType.type] = newMimeType

        Reflect.defineProperty(newMimeType, 'enabledPlugin', {
          value: newPlugin,
        })
      }

      // Patch `Plugin.item(index)` function to return the correct item otherwise it
      // throws a `TypeError: Can only call Plugin.item on instances of Plugin`
      newPlugin.item = function (index: number) {
        return newPlugin[index] as MimeType
      }

      return newPlugin
    }

    if (window.navigator.plugins !== undefined) {
      // We need the original length so we can reference it (as we will change it)
      const plugins = window.navigator.plugins
      const originalPluginsLength = plugins.length
      const pluginsPrototype: Record<string | number, unknown> =
        Object.getPrototypeOf(window.navigator.plugins)

      const fakePlugins = fakePluginData.map((pluginData) => {
        return makeFakePlugin(pluginData)
      })

      // Adds a fake plugin for the given index on fakePluginData
      fakePlugins.forEach((newPlugin, index) => {
        const pluginPosition = originalPluginsLength + index
        pluginsPrototype[pluginPosition] = newPlugin
        pluginsPrototype[newPlugin.name] = newPlugin
      })

      // Farble the `item` method on the plugins array
      const originalItem = window.navigator.plugins.item
      pluginsPrototype['item'] = function (index: number, ...rest: unknown[]) {
        if (index < originalPluginsLength) {
          return Reflect.apply(originalItem as Function, this, [
            index,
            ...rest,
          ]) as Plugin
        } else {
          const farbledIndex = index - originalPluginsLength
          return fakePlugins[farbledIndex]
        }
      }

      // Farble the `namedItem` method on the plugins array
      const originalNamedItem = window.navigator.plugins.namedItem
      pluginsPrototype['namedItem'] = function (
        name: string,
        ...rest: unknown[]
      ) {
        let namedPlugin = Reflect.apply(originalNamedItem as Function, this, [
          name,
          ...rest,
        ]) as Plugin | null
        if (namedPlugin) {
          return namedPlugin
        }
        return fakePlugins.find((plugin) => plugin.name === name)
      }

      // Adjust the length of the original plugin array
      Reflect.defineProperty(pluginsPrototype, 'length', {
        value: originalPluginsLength + fakePlugins.length,
      })
    }
  }

  // 3. Farble speech synthesizer
  // Adds a vake voice determined by the fakeVoiceName and randomVoiceIndexScale.
  const farbleVoices = (
    fakeVoiceName: string,
    randomVoiceIndexScale: number,
  ) => {
    if (window.SpeechSynthesisUtterance === undefined) {
      return
    }
    const makeFakeVoiceFromVoice = (voice: SpeechSynthesisVoice) => {
      const newVoice = Object.create(Object.getPrototypeOf(voice), {
        name: { value: fakeVoiceName },
        voiceURI: { value: voice.voiceURI },
        lang: { value: voice.lang },
        localService: { value: voice.localService },
        default: { value: false },
      })

      return newVoice as SpeechSynthesisVoice
    }

    let originalVoice: SpeechSynthesisVoice | undefined
    let fakeVoice: SpeechSynthesisVoice | undefined
    let passedFakeVoice: SpeechSynthesisVoice | undefined

    // We need to override the voice property to allow our fake voice to work
    const descriptor = Reflect.getOwnPropertyDescriptor(
      SpeechSynthesisUtterance.prototype,
      'voice',
    )!
    Reflect.defineProperty(SpeechSynthesisUtterance.prototype, 'voice', {
      get(...args: unknown[]) {
        if (!passedFakeVoice) {
          // We didn't set a fake voice
          return Reflect.apply(descriptor.get!, this, args)
        } else {
          // We set a fake voice, return that instead
          return passedFakeVoice
        }
      },
      set(passedVoice: SpeechSynthesisVoice) {
        if (passedVoice === fakeVoice && originalVoice !== undefined) {
          // If we passed a fake voice, ignore it. We need to use the real voice
          // The fake voice will not work.
          passedFakeVoice = passedVoice
          Reflect.apply(descriptor.set!, this, [originalVoice])
        } else {
          // Otherwise, if we set a real voice, use a real voice instead.
          passedFakeVoice = undefined
          Reflect.apply(descriptor.set!, this, [passedVoice])
        }
      },
    })

    // Patch get voices to return an extra fake voice
    const getVoices = window.speechSynthesis.getVoices
    const getVoicesPrototype: Record<string, Function> = Object.getPrototypeOf(
      window.speechSynthesis,
    )
    getVoicesPrototype['getVoices'] = function (...args: unknown[]) {
      const voices = Reflect.apply(
        getVoices,
        this,
        args,
      ) as SpeechSynthesisVoice[]

      if (fakeVoice === undefined) {
        const randomVoiceIndex = Math.round(
          randomVoiceIndexScale * voices.length,
        )
        originalVoice = voices[randomVoiceIndex]
        if (originalVoice !== undefined) {
          fakeVoice = makeFakeVoiceFromVoice(originalVoice)
        }

        if (fakeVoice !== undefined) {
          voices.push(fakeVoice)
        }
      } else {
        voices.push(fakeVoice)
      }
      return voices
    }
  }

  // 4. Farble hardwareConcurrency
  // Adds a random value between 2 and the original hardware concurrency
  // using the provided `randomHardwareIndexScale` which must be a random
  // value between 0 and 1.
  const farbleHardwareConcurrency = (randomHardwareIndexScale: number) => {
    const hardwareConcurrency = window.navigator.hardwareConcurrency
    // We only farble amounts greater than 2
    if (hardwareConcurrency <= 2) {
      return
    }
    const remaining = hardwareConcurrency - 2
    const newRemaining = Math.round(remaining * randomHardwareIndexScale)

    Reflect.defineProperty(window.navigator, 'hardwareConcurrency', {
      value: newRemaining + 2,
    })
  }

  try {
    // A value between 0.99 and 1 to fudge the audio data
    // A value between 0.99 to 1 means the values in the destination will
    // always be within the expected range of -1 and 1.
    // This small decrease should not affect affect legitimite users of this api.
    // But will affect fingerprinters by introducing a small random change.
    const fudgeFactor = args['fudgeFactor']
    farbleAudio(fudgeFactor)
  } catch (error) {
    console.error(`Failed to farble audio: ${error}`)
  }

  try {
    // Fake data that is to be used to construct fake plugins
    const fakePluginData = args['fakePluginData']
    farblePlugins(fakePluginData)
  } catch (error) {
    console.error(`Failed to farble plugins: ${error}`)
  }

  try {
    // A value representing a fake voice name that will be used to add a
    // fake voice
    const fakeVoiceName = args['fakeVoiceName']
    // This value is used to get a random index between 0 and voices.length
    // It's important to have a value between 0 - 1 in order to be within the
    // array bounds
    const randomVoiceIndexScale = args['randomVoiceIndexScale']
    farbleVoices(fakeVoiceName, randomVoiceIndexScale)
  } catch (error) {
    console.error(`Failed to farble voices: ${error}`)
  }

  try {
    // This value lets us pick a value between 2 and
    // window.navigator.hardwareConcurrency. It is a value between 0 and 1.
    // For example 0.5 will give us 3 and thus return 2 + 3 = 5 for hardware
    // concurrency
    const randomHardwareIndexScale = args['randomHardwareIndexScale']
    farbleHardwareConcurrency(randomHardwareIndexScale)
  } catch (error) {
    console.error(`Failed to farble hardware concurrency: ${error}`)
  }
}
