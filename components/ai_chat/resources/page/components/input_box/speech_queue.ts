// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Leave ample room under the 5 MiB audio limit, even for slow speech.
export const MAX_SPEECH_CHUNK_LENGTH = 240

/** Queues complete sentences from an accumulating reply for ordered playback. */
export class SpeechQueue {
  private offset = 0
  private turn: string | undefined
  private generation = 0
  private pending = Promise.resolve()
  private error: Error | undefined

  constructor(private readonly speak: (text: string) => Promise<void>) {}

  private async speakChunk(text: string, generation: number): Promise<void> {
    if (generation !== this.generation) {
      return
    }
    try {
      await this.speak(text)
    } catch (error) {
      if (
        !(error instanceof Error)
        || error.message !== 'tts-audio-too-large'
      ) {
        throw error
      }
      const characters = Array.from(text)
      if (characters.length < 2) {
        throw error
      }
      const middle = Math.floor(characters.length / 2)
      await this.speakChunk(characters.slice(0, middle).join(''), generation)
      await this.speakChunk(characters.slice(middle).join(''), generation)
    }
  }

  reset() {
    this.generation++
    this.offset = 0
    this.turn = undefined
    this.pending = Promise.resolve()
    this.error = undefined
  }

  append(text: string, final = false, turn?: string) {
    if (turn !== this.turn) {
      this.offset = 0
      this.turn = turn
    }
    // Hide code while it is streaming as well as after its closing fence.
    const cleaned = text
      .replace(/```[\s\S]*?(?:```|$)/g, '')
      .replace(/\[([^\]]+)\]\([^)]+\)/g, '$1')
      .replace(/https?:\/\/\S+/g, '')
      .replace(/[*_~`#>]/g, '')
    let remainder = cleaned.slice(this.offset)
    while (remainder.length) {
      const sentence = /[.!?](?:["')\]]*)(?:\s+|$)/.exec(remainder)
      let end = sentence ? sentence.index + sentence[0].length : 0
      if (!end && !final && remainder.length <= MAX_SPEECH_CHUNK_LENGTH) {
        break
      }
      end ||= remainder.length
      if (end > MAX_SPEECH_CHUNK_LENGTH) {
        end = MAX_SPEECH_CHUNK_LENGTH
        const space = remainder.lastIndexOf(' ', end - 1)
        if (space > 0) {
          end = space + 1
        } else if (/^[\uDC00-\uDFFF]$/.test(remainder[end])) {
          end--
        }
      }
      const chunk = remainder.slice(0, end).trim()
      this.offset += end
      remainder = remainder.slice(end)
      if (!chunk) {
        continue
      }
      const generation = this.generation
      this.pending = this.pending.then(async () => {
        if (generation === this.generation && !this.error) {
          try {
            await this.speakChunk(chunk, generation)
          } catch (error) {
            if (generation === this.generation) {
              this.error =
                error instanceof Error ? error : new Error(String(error))
            }
          }
        }
      })
    }
  }

  async wait() {
    await this.pending
    if (this.error) {
      throw this.error
    }
  }
}
