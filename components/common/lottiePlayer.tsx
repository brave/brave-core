// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

let workerLoaderPolicy: any | null = null;

function getLottieWorkerURL() {
  if (workerLoaderPolicy === null) {
    workerLoaderPolicy =
      (window as any).trustedTypes!.createPolicy('lottie-worker-script-loader', {
        createScriptURL: (_ignore: string) => {
          const script =
            `import 'chrome://resources/lottie/lottie_worker.min.js';`;
          // CORS blocks loading worker script from a different origin, even
          // if chrome://resources/ is added in the 'worker-src' CSP header.
          // (see https://crbug.com/1385477). Loading scripts as blob and then
          // instantiating it as web worker is possible.
          const blob = new Blob([script], { type: 'text/javascript' });
          return URL.createObjectURL(blob);
        },
        createHTML: () => { throw new Error('Should not be called') },
        createScript: () => { throw new Error('Should not be called') },
      });
  }

  return workerLoaderPolicy.createScriptURL('');
}

function loadAnimation(maybeValidUrl: string) {
  const url = new URL(maybeValidUrl, document.location.href);
  if (url.protocol !== 'chrome:' &&
    (url.protocol !== 'data:' || !url.pathname.startsWith('application/json;'))) {
    throw new Error(`Invalid scheme or data url used. ${maybeValidUrl}`)
  }

  const { promise, resolve } = Promise.withResolvers<object | null | string>()
  const xhr = new XMLHttpRequest()
  xhr.open('GET', url, true)
  xhr.responseType = 'json'
  xhr.send()
  xhr.onreadystatechange = () => {
    if (xhr.readyState === 4 && xhr.status === 200) {
      resolve(xhr.response)
    }
  }
  return promise
}

interface MessageData {
  animationData: object | null | string;
  drawSize: { width: number, height: number };
  params: { loop: boolean, autoplay: boolean };
  canvas?: OffscreenCanvas;
}

interface LottiePlayerProps {
  animationUrl: string
  play: boolean
  loop?: boolean
  onInitialized?: () => void
  onPlaying?: () => void
  onPaused?: () => void
  onStopped?: () => void
  onComplete?: () => void
  onResized?: (size: { width: number, height: number }) => void
}

class LottiePlayerState {
  private worker: Worker
  private resizeObserver: ResizeObserver

  // Promises for waiting for the worker to be initialized with the animation data.
  private resolveInit: () => void
  private initPromise: Promise<void> = new Promise((resolve) => this.resolveInit = resolve)

  // The last props passed to the component. These are required for notifying listeners via the onXXX methods.
  accessor props: LottiePlayerProps

  private isPlaying: boolean = false;
  get play() {
    return this.isPlaying
  }

  set play(value: boolean) {
    if (this.isPlaying === value) return

    this.isPlaying = value
    this.initPromise?.then(() => {
      this.worker.postMessage({ control: { play: this.isPlaying } })
    })
  }

  private url: string
  get animationUrl() {
    return this.url
  }

  set animationUrl(value: string) {
    if (this.url === value) return

    // When we change the animation URL we need to wait for the animation to be initialized
    // before we can send the play control information to the worker.
    const resolvable = Promise.withResolvers<void>()
    this.initPromise = resolvable.promise

    // Note: This is resolved by the worker when the animation is initialized.
    this.resolveInit = resolvable.resolve
    this.url = value

    loadAnimation(value).then(data => {
      const message: MessageData = {
        animationData: data,
        drawSize: this.getCanvasSize(),
        params: { loop: !!this.props.loop, autoplay: this.play },
      }

      if (!this.canvas.dataset.transferred) {
        message.canvas = this.canvas.transferControlToOffscreen()
        this.canvas.dataset.transferred = 'true'
        this.worker.postMessage(message, [message.canvas])
      } else {
        this.worker.postMessage(message)
      }
    })
  }

  constructor(private readonly canvas: HTMLCanvasElement, autoplay: boolean) {
    this.isPlaying = autoplay
    this.worker = new Worker(getLottieWorkerURL() as unknown as URL, { type: 'module' })
    this.worker.onmessage = (event: MessageEvent) => {
      if (!this.props) {
        return
      }

      if (event.data.name === 'initialized' && event.data.success) {
        this.props.onInitialized?.()
        this.resolveInit()
      } else if (event.data.name === 'playing') {
        this.props.onPlaying?.()
      } else if (event.data.name === 'paused') {
        this.props.onPaused?.()
      } else if (event.data.name === 'stopped') {
        this.props.onStopped?.()
      } else if (event.data.name === 'complete') {
        this.props.onComplete?.()
      } else if (event.data.name === 'resized') {
        this.props.onResized?.(event.data.size)
      }
    }

    this.resizeObserver = new ResizeObserver(e => {
      this.initPromise?.then(() => {
        this.worker.postMessage({ drawSize: this.getCanvasSize() })
      })
    })
    this.resizeObserver.observe(this.canvas)
  }

  private getCanvasSize(): { width: number, height: number } {
    const canvasElement = this.canvas;
    const devicePixelRatio = window.devicePixelRatio;
    const clientRect = canvasElement.getBoundingClientRect();
    const drawSize = {
      width: clientRect.width * devicePixelRatio,
      height: clientRect.height * devicePixelRatio,
    };
    return drawSize;
  }

  destroy() {
    this.worker.terminate()
    this.resizeObserver.disconnect()
    this.resolveInit = () => { }
  }
}

const canvasStyle = {
  width: '100%',
  height: '100%'
} as const

function LottiePlayer(props: LottiePlayerProps) {
  const [state, setState] = React.useState<LottiePlayerState | null>(null)
  const ref = React.useRef<HTMLCanvasElement>(null)

  // Make sure the state has an up to date set of props for notifying listeners via the onXXX methods.
  if (state) {
    state.props = props
    state.animationUrl = props.animationUrl
    state.play = props.play
  }

  // Handle creating the worker and cleaning up when the component unmounts
  React.useEffect(() => {
    if (!ref.current) throw new Error("Canvas element not found")

    const state = new LottiePlayerState(ref.current, props.play)
    setState(state)

    return () => {
      state.destroy()
    }
  }, [])

  return <canvas ref={ref} style={canvasStyle} />
}

export default LottiePlayer
