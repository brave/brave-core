// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'

interface Props {
  modelUrl: string
  description: string
  posterUrl?: string
}

type CustomElement<T> = Partial<T & React.DOMAttributes<T> & { children: any }>
declare global {
  namespace JSX {
    interface IntrinsicElements {
      ['model-viewer']: CustomElement<any>
    }
  }
}

const modelViewerScriptUrl = 'https://unpkg.com/@google/model-viewer@2.0.0/dist/model-viewer.min.js'

export const ModelViewer = (props: Props) => {
  const {
    modelUrl,
    description,
    posterUrl
  } = props

  // state
  const [scriptLoaded, setScriptLoaded] = React.useState<boolean>(false)

  React.useEffect(() => {
    const scriptId = 'model-viewer-js'

    // check if script exists in DOM
    if (document.querySelector(`script#${scriptId}`) === null) {
      const script = document.createElement('script')
      script.id = scriptId
      script.src = modelViewerScriptUrl
      script.type = 'module'
      script.onload = () => setScriptLoaded(true)
      document.body.append(script)
    }
  }, [])

  return (
    <>
      {scriptLoaded &&
        <model-viewer
          alt={description}
          src={modelUrl}
          camera-controls="true"
          touch-action="pan-y"
          auto-rotate="true"
          auto-play="true"
          ar-status="not-presenting"
          progress={(event: any) => console.log(event.detail.totalProgress)}
          poster={posterUrl || ''}
        />
      }
    </>
  )
}
