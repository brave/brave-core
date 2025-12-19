// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

type SSEEvent = {
  event: string
  data: string
}

async function readSSE(response: Response, onEvent: (event: SSEEvent) => void) {
  const reader = response.body!.getReader()
  const decoder = new TextDecoder()

  let buffer = ''
  let event = 'message'
  let data = ''

  while (true) {
    const { value, done } = await reader.read()
    if (done) break

    buffer += decoder.decode(value, { stream: true })

    while (true) {
      const newlineIndex = buffer.indexOf('\n')
      if (newlineIndex === -1) break

      const line = buffer.slice(0, newlineIndex).trimEnd()
      buffer = buffer.slice(newlineIndex + 1)

      if (!line) {
        if (data) {
          onEvent({ event, data })
          data = ''
          event = 'message'
        }
        continue
      }

      if (line.startsWith('event:')) {
        event = line.slice(6).trim()
      } else if (line.startsWith('data:')) {
        data += line.slice(5).trim() + '\n'
      }
    }
  }
}

// This is an example of how to perform a request using the above readSSE helper
// A real implementation would accept callbacks for chunk-received and completed
// as well as parameters needed to construct the request
async function PerformRequest() {
  const res = await fetch('http://localhost:8000/v1/conversation', {
    method: 'POST',
    headers: {
      Accept: 'text/event-stream',
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({
      model: 'qwen-14b-instruct',
      stream: true,
      events: [{ role: 'user', type: 'chatMessage', content: 'Hello' }],
    }),
  })

  // TODO: handle error response

  await readSSE(res, ({ event, data }) => {
    console.log('read sse', { event, data })
    if (data.startsWith('[DONE]')) return

    const json = JSON.parse(data)

    if (event === 'message') {
      const delta = json.choices?.[0]?.delta?.content
      if (delta) {
        console.log(delta) // call callback
      }
    }

    if (event === 'tool') {
      console.log('Tool event:', json)
    }
  })

  // TODO: call completed callback
}
