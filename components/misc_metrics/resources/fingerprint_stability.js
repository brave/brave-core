// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
const hashValue = (() => {
  // Serializes values with sorted object keys to produce a stable string for hashing.
  function toCanonicalString(value) {
    return JSON.stringify(value, (_, val) => {
      if (val && typeof val === 'object' && !Array.isArray(val)) {
        return Object.keys(val)
          .sort()
          .reduce((sorted, k) => {
            sorted[k] = val[k]
            return sorted
          }, {})
      }
      return val
    })
  }

  // FNV-1a hash, using the standard parameters for the 32-bit variant
  return (value) => {
    const str = toCanonicalString(value)
    let h = 0x811c9dc5
    for (let i = 0; i < str.length; i++) {
      h ^= str.charCodeAt(i)
      h = Math.imul(h, 0x01000193)
    }
    return h & 0x7fffffff
  }
})()

const { getWebGlBasics, getWebGlExtensions } = (() => {
  /** WebGl context is not available */
  const STATUS_NO_GL_CONTEXT = -1
  /** WebGL context `getParameter` method is not a function */
  const STATUS_GET_PARAMETER_NOT_A_FUNCTION = -2

  /**
   * WebGL parameter enum values that can be queried via gl.getParameter().
   * Context parameters describe GPU hardware limits and graphics settings (e.g. max texture size, viewport bounds).
   */
  const validContextParameters = new Set([
    10752, 2849, 2884, 2885, 2886, 2928, 2929, 2930, 2931, 2932, 2960, 2961,
    2962, 2963, 2964, 2965, 2966, 2967, 2968, 2978, 3024, 3042, 3088, 3089,
    3106, 3107, 32773, 32777, 32777, 32823, 32824, 32936, 32937, 32938, 32939,
    32968, 32969, 32970, 32971, 3317, 33170, 3333, 3379, 3386, 33901, 33902,
    34016, 34024, 34076, 3408, 3410, 3411, 3412, 3413, 3414, 3415, 34467, 34816,
    34817, 34818, 34819, 34877, 34921, 34930, 35660, 35661, 35724, 35738, 35739,
    36003, 36004, 36005, 36347, 36348, 36349, 37440, 37441, 37443, 7936, 7937,
    7938,
  ])
  const validExtensionParams = new Set([
    34047, // MAX_TEXTURE_MAX_ANISOTROPY_EXT
    35723, // FRAGMENT_SHADER_DERIVATIVE_HINT_OES
    36063, // MAX_COLOR_ATTACHMENTS_WEBGL
    34852, // MAX_DRAW_BUFFERS_WEBGL
    34853, // DRAW_BUFFER0_WEBGL
    34854, // DRAW_BUFFER1_WEBGL
    34229, // VERTEX_ARRAY_BINDING_OES
    36392, // TIMESTAMP_EXT
    36795, // GPU_DISJOINT_EXT
    38449, // MAX_VIEWS_OVR
  ])
  const shaderTypes = ['FRAGMENT_SHADER', 'VERTEX_SHADER']
  const precisionTypes = [
    'LOW_FLOAT',
    'MEDIUM_FLOAT',
    'HIGH_FLOAT',
    'LOW_INT',
    'MEDIUM_INT',
    'HIGH_INT',
  ]
  const rendererInfoExtensionName = 'WEBGL_debug_renderer_info'
  const polygonModeExtensionName = 'WEBGL_polygon_mode'

  /**
   * Gets the basic and simple WebGL parameters
   */
  function getWebGlBasics() {
    const gl = getWebGLContext()
    if (!gl) {
      return STATUS_NO_GL_CONTEXT
    }

    if (!isValidParameterGetter(gl)) {
      return STATUS_GET_PARAMETER_NOT_A_FUNCTION
    }

    const debugExtension = gl.getExtension(rendererInfoExtensionName)

    return {
      version: gl.getParameter(gl.VERSION)?.toString() || '',
      vendor: gl.getParameter(gl.VENDOR)?.toString() || '',
      vendorUnmasked: debugExtension
        ? gl.getParameter(debugExtension.UNMASKED_VENDOR_WEBGL)?.toString()
        : '',
      renderer: gl.getParameter(gl.RENDERER)?.toString() || '',
      rendererUnmasked: debugExtension
        ? gl.getParameter(debugExtension.UNMASKED_RENDERER_WEBGL)?.toString()
        : '',
      shadingLanguageVersion:
        gl.getParameter(gl.SHADING_LANGUAGE_VERSION)?.toString() || '',
    }
  }

  /**
   * Gets the advanced and massive WebGL parameters and extensions
   */
  function getWebGlExtensions() {
    const gl = getWebGLContext()
    if (!gl) {
      return STATUS_NO_GL_CONTEXT
    }

    if (!isValidParameterGetter(gl)) {
      return STATUS_GET_PARAMETER_NOT_A_FUNCTION
    }

    const extensions = gl.getSupportedExtensions()
    const contextAttributes = gl.getContextAttributes()
    const unsupportedExtensions = []

    // Features
    const attributes = []
    const parameters = []
    const extensionParameters = []
    const shaderPrecisions = []

    // Context attributes
    if (contextAttributes) {
      for (const attributeName of Object.keys(contextAttributes)) {
        attributes.push(`${attributeName}=${contextAttributes[attributeName]}`)
      }
    }

    // Context parameters
    const constants = getConstantsFromPrototype(gl)
    for (const constant of constants) {
      const code = gl[constant]
      parameters.push(
        `${constant}=${code}${validContextParameters.has(code) ? `=${gl.getParameter(code)}` : ''}`,
      )
    }

    // Extension parameters
    if (extensions) {
      for (const name of extensions) {
        if (
          name === rendererInfoExtensionName
          || name === polygonModeExtensionName
        ) {
          continue
        }

        const extension = gl.getExtension(name)
        if (!extension) {
          unsupportedExtensions.push(name)
          continue
        }

        for (const constant of getConstantsFromPrototype(extension)) {
          const code = extension[constant]
          extensionParameters.push(
            `${constant}=${code}${validExtensionParams.has(code) ? `=${gl.getParameter(code)}` : ''}`,
          )
        }
      }
    }

    // Shader precision
    for (const shaderType of shaderTypes) {
      for (const precisionType of precisionTypes) {
        const shaderPrecision = getShaderPrecision(
          gl,
          shaderType,
          precisionType,
        )
        shaderPrecisions.push(
          `${shaderType}.${precisionType}=${shaderPrecision.join(',')}`,
        )
      }
    }

    // Postprocess
    extensionParameters.sort()
    parameters.sort()

    return {
      contextAttributes: attributes,
      parameters: parameters,
      shaderPrecisions: shaderPrecisions,
      extensions: extensions,
      extensionParameters: extensionParameters,
      unsupportedExtensions,
    }
  }

  function getWebGLContext() {
    const canvas = document.createElement('canvas')
    let context

    canvas.addEventListener(
      'webglCreateContextError',
      () => (context = undefined),
    )

    for (const type of ['webgl', 'experimental-webgl']) {
      try {
        context = canvas.getContext(type)
      } catch {
        // Ok, continue
      }
      if (context) {
        break
      }
    }

    return context
  }

  function getShaderPrecision(gl, shaderType, precisionType) {
    const shaderPrecision = gl.getShaderPrecisionFormat(
      gl[shaderType],
      gl[precisionType],
    )
    return shaderPrecision
      ? [
          shaderPrecision.rangeMin,
          shaderPrecision.rangeMax,
          shaderPrecision.precision,
        ]
      : []
  }

  function getConstantsFromPrototype(obj) {
    const keys = Object.keys(obj.__proto__)
    return keys.filter(isConstantLike)
  }

  function isConstantLike(key) {
    return typeof key === 'string' && !key.match(/[^A-Z0-9_x]/)
  }

  function isValidParameterGetter(gl) {
    return typeof gl.getParameter === 'function'
  }

  return {
    getWebGlBasics,
    getWebGlExtensions,
  }
})()

const getAudioFingerprintPromise = (() => {
  function isPromise(value) {
    return !!value && typeof value.then === 'function'
  }

  const suppressUnhandledRejectionWarning = (promise) => {
    promise.then(undefined, () => undefined)
    return promise
  }

  const SpecialFingerprint = {
    KnownForSuspending: -1,
    NotSupported: -2,
    Timeout: -3,
    KnownForAntifingerprinting: -4,
  }

  const InnerErrorName = {
    Timeout: 'timeout',
    Suspended: 'suspended',
  }

  const getUnstableAudioFingerprint = () => {
    const w = window
    const AudioContext = w.OfflineAudioContext || w.webkitOfflineAudioContext
    if (!AudioContext) {
      return SpecialFingerprint.NotSupported
    }

    const hashFromIndex = 4500
    const hashToIndex = 5000
    const context = new AudioContext(1, hashToIndex, 44100)

    const oscillator = context.createOscillator()
    oscillator.type = 'triangle'
    oscillator.frequency.value = 10000

    const compressor = context.createDynamicsCompressor()
    compressor.threshold.value = -50
    compressor.knee.value = 40
    compressor.ratio.value = 12
    compressor.attack.value = 0
    compressor.release.value = 0.25

    oscillator.connect(compressor)
    compressor.connect(context.destination)
    oscillator.start(0)

    const [renderPromise, finishRendering] = startRenderingAudio(context)
    const fingerprintPromise = suppressUnhandledRejectionWarning(
      renderPromise.then(
        (buffer) => getHash(buffer.getChannelData(0).subarray(hashFromIndex)),
        (error) => {
          if (
            error.name === InnerErrorName.Timeout
            || error.name === InnerErrorName.Suspended
          ) {
            return SpecialFingerprint.Timeout
          }
          throw error
        },
      ),
    )

    return () => {
      finishRendering()
      return fingerprintPromise
    }
  }

  function startRenderingAudio(context) {
    const renderTryMaxCount = 3
    const renderRetryDelay = 500
    const runningMaxAwaitTime = 500
    const runningSufficientTime = 5000
    let finalize = () => undefined

    const resultPromise = new Promise((resolve, reject) => {
      let isFinalized = false
      let renderTryCount = 0
      let startedRunningAt = 0

      context.oncomplete = (event) => resolve(event.renderedBuffer)

      const startRunningTimeout = () => {
        setTimeout(
          () => reject(makeInnerError(InnerErrorName.Timeout)),
          Math.min(
            runningMaxAwaitTime,
            startedRunningAt + runningSufficientTime - Date.now(),
          ),
        )
      }

      const tryRender = () => {
        try {
          const renderingPromise = context.startRendering()

          if (isPromise(renderingPromise)) {
            suppressUnhandledRejectionWarning(renderingPromise)
          }

          switch (context.state) {
            case 'running':
              startedRunningAt = Date.now()
              if (isFinalized) {
                startRunningTimeout()
              }
              break

            case 'suspended':
              renderTryCount++
              if (isFinalized && renderTryCount >= renderTryMaxCount) {
                reject(makeInnerError(InnerErrorName.Suspended))
              } else {
                setTimeout(tryRender, renderRetryDelay)
              }
              break
          }
        } catch (error) {
          reject(error)
        }
      }

      tryRender()

      finalize = () => {
        if (!isFinalized) {
          isFinalized = true
          if (startedRunningAt > 0) {
            startRunningTimeout()
          }
        }
      }
    })

    return [resultPromise, finalize]
  }

  function getHash(signal) {
    let hash = 0
    for (let i = 0; i < signal.length; ++i) {
      hash += Math.abs(signal[i])
    }
    return hash
  }

  function makeInnerError(name) {
    const error = new Error(name)
    error.name = name
    return error
  }

  return () => {
    const result = getUnstableAudioFingerprint()
    const promise = Number.isInteger(result)
      ? Promise.resolve(result)
      : result()
    return Promise.race([
      promise,
      new Promise((resolve) =>
        setTimeout(() => resolve(SpecialFingerprint.Timeout), 3000),
      ),
    ])
  }
})()

const getFontsFingerprintPromise = (() => {
  const wait = (durationMs, resolveWith) => {
    return new Promise((resolve) =>
      setTimeout(resolve, durationMs, resolveWith),
    )
  }

  const withIframe = async (action, initialHtml, domPollInterval = 50) => {
    const d = document

    while (!d.body) {
      await wait(domPollInterval)
    }

    const iframe = d.createElement('iframe')

    try {
      await new Promise((_resolve, _reject) => {
        let isComplete = false
        const resolve = () => {
          isComplete = true
          _resolve()
        }
        const reject = (error) => {
          isComplete = true
          _reject(error)
        }

        iframe.onload = resolve
        iframe.onerror = reject
        const { style } = iframe
        style.setProperty('display', 'block', 'important')
        style.position = 'absolute'
        style.top = '0'
        style.left = '0'
        style.visibility = 'hidden'
        if (initialHtml && 'srcdoc' in iframe) {
          iframe.srcdoc = initialHtml
        } else {
          iframe.src = 'about:blank'
        }
        d.body.appendChild(iframe)

        const checkReadyState = () => {
          if (isComplete) {
            return
          }

          if (iframe.contentWindow?.document?.readyState === 'complete') {
            resolve()
          } else {
            setTimeout(checkReadyState, 10)
          }
        }
        checkReadyState()
      })

      while (!iframe.contentWindow?.document?.body) {
        await wait(domPollInterval)
      }

      return await action(iframe, iframe.contentWindow)
    } finally {
      iframe.parentNode?.removeChild(iframe)
    }
  }

  const testString = 'mmMwWLliI0O&1'
  const textSize = '48px'
  const baseFonts = ['monospace', 'sans-serif', 'serif']

  const fontList = [
    'sans-serif-thin',
    'ARNO PRO',
    'Agency FB',
    'Arabic Typesetting',
    'Arial Unicode MS',
    'AvantGarde Bk BT',
    'BankGothic Md BT',
    'Batang',
    'Bitstream Vera Sans Mono',
    'Calibri',
    'Century',
    'Century Gothic',
    'Clarendon',
    'EUROSTILE',
    'Franklin Gothic',
    'Futura Bk BT',
    'Futura Md BT',
    'GOTHAM',
    'Gill Sans',
    'HELV',
    'Haettenschweiler',
    'Helvetica Neue',
    'Humanst521 BT',
    'Leelawadee',
    'Letter Gothic',
    'Levenim MT',
    'Lucida Bright',
    'Lucida Sans',
    'Menlo',
    'MS Mincho',
    'MS Outlook',
    'MS Reference Specialty',
    'MS UI Gothic',
    'MT Extra',
    'MYRIAD PRO',
    'Marlett',
    'Meiryo UI',
    'Microsoft Uighur',
    'Minion Pro',
    'Monotype Corsiva',
    'PMingLiU',
    'Pristina',
    'SCRIPTINA',
    'Segoe UI Light',
    'Serifa',
    'SimHei',
    'Small Fonts',
    'Staccato222 BT',
    'TRAJAN PRO',
    'Univers CE 55 Medium',
    'Vrinda',
    'ZWAdobeF',
  ]

  const getFonts = () => {
    return withIframe(async (_, { document }) => {
      const holder = document.body
      holder.style.fontSize = textSize

      const spansContainer = document.createElement('div')
      spansContainer.style.setProperty('visibility', 'hidden', 'important')

      const defaultWidth = {}
      const defaultHeight = {}

      const createSpan = (fontFamily) => {
        const span = document.createElement('span')
        const { style } = span
        style.position = 'absolute'
        style.top = '0'
        style.left = '0'
        style.fontFamily = fontFamily
        span.textContent = testString
        spansContainer.appendChild(span)
        return span
      }

      const createSpanWithFonts = (fontToDetect, baseFont) => {
        return createSpan(`'${fontToDetect}',${baseFont}`)
      }

      const initializeBaseFontsSpans = () => {
        return baseFonts.map(createSpan)
      }

      const initializeFontsSpans = () => {
        const spans = {}
        for (const font of fontList) {
          spans[font] = baseFonts.map((baseFont) =>
            createSpanWithFonts(font, baseFont),
          )
        }
        return spans
      }

      const isFontAvailable = (fontSpans) => {
        return baseFonts.some(
          (baseFont, baseFontIndex) =>
            fontSpans[baseFontIndex].offsetWidth !== defaultWidth[baseFont]
            || fontSpans[baseFontIndex].offsetHeight
              !== defaultHeight[baseFont],
        )
      }

      const baseFontsSpans = initializeBaseFontsSpans()
      const fontsSpans = initializeFontsSpans()
      holder.appendChild(spansContainer)

      for (let index = 0; index < baseFonts.length; index++) {
        defaultWidth[baseFonts[index]] = baseFontsSpans[index].offsetWidth
        defaultHeight[baseFonts[index]] = baseFontsSpans[index].offsetHeight
      }

      return fontList.filter((font) => isFontAvailable(fontsSpans[font]))
    })
  }

  return getFonts
})()

const getCanvasFingerprint = (() => {
  const ImageStatus = {
    Unstable: -1,
  }

  const getCanvasFingerprint = () => {
    return getUnstableCanvasFingerprint()
  }

  const getUnstableCanvasFingerprint = () => {
    const [canvas, context] = makeCanvasContext()
    const winding = doesSupportWinding(context)
    const [geometry, text] = renderImages(canvas, context)
    return { winding, geometry, text }
  }

  function makeCanvasContext() {
    const canvas = document.createElement('canvas')
    canvas.width = 1
    canvas.height = 1
    return [canvas, canvas.getContext('2d')]
  }

  function doesSupportWinding(context) {
    context.rect(0, 0, 10, 10)
    context.rect(2, 2, 6, 6)
    return !context.isPointInPath(5, 5, 'evenodd')
  }

  function renderImages(canvas, context) {
    renderTextImage(canvas, context)
    const textImage1 = canvasToString(canvas)
    const textImage2 = canvasToString(canvas)

    if (textImage1 !== textImage2) {
      return [ImageStatus.Unstable, ImageStatus.Unstable]
    }

    renderGeometryImage(canvas, context)
    const geometryImage = canvasToString(canvas)
    return [geometryImage, textImage1]
  }

  function renderTextImage(canvas, context) {
    canvas.width = 240
    canvas.height = 60

    context.textBaseline = 'alphabetic'
    context.fillStyle = '#f60'
    context.fillRect(100, 1, 62, 20)

    context.fillStyle = '#069'
    context.font = '11pt "Times New Roman"'
    const printedText = `Cwm fjordbank gly ${String.fromCharCode(55357, 56835) /* 😃 */}`
    context.fillText(printedText, 2, 15)
    context.fillStyle = 'rgba(102, 204, 0, 0.2)'
    context.font = '18pt Arial'
    context.fillText(printedText, 4, 45)
  }

  function renderGeometryImage(canvas, context) {
    canvas.width = 122
    canvas.height = 110

    context.globalCompositeOperation = 'multiply'
    for (const [color, x, y] of [
      ['#f2f', 40, 40],
      ['#2ff', 80, 40],
      ['#ff2', 60, 80],
    ]) {
      context.fillStyle = color
      context.beginPath()
      context.arc(x, y, 40, 0, Math.PI * 2, true)
      context.closePath()
      context.fill()
    }

    context.fillStyle = '#f9c'
    context.arc(60, 60, 60, 0, Math.PI * 2, true)
    context.arc(60, 60, 20, 0, Math.PI * 2, true)
    context.fill('evenodd')
  }

  function canvasToString(canvas) {
    return canvas.toDataURL()
  }

  return getCanvasFingerprint
})()

;(async () => {
  const webglBasics = getWebGlBasics()
  const rawData = {
    canvas: getCanvasFingerprint(),
    fonts: await getFontsFingerprintPromise(),
    timezone: Intl.DateTimeFormat().resolvedOptions().timeZone,
    navigator_deviceMemory: navigator.deviceMemory,
    navigator_hardwareConcurrency: navigator.hardwareConcurrency,
    navigator_languages: navigator.languages,
    navigator_userAgent: navigator.userAgent,
    screenAvailSize: { height: screen.availHeight, width: screen.availWidth },
    screenSize: { height: screen.height, width: screen.width },
    screen_pixelDepth: screen.pixelDepth,
    webAudio: await getAudioFingerprintPromise(),
    webglExtensions: getWebGlExtensions(),
    webglRendererUnmasked: webglBasics.rendererUnmasked,
    webglVendorUnmasked: webglBasics.vendorUnmasked,
    windowDevicePixelRatio: window.devicePixelRatio,
  }

  const processedData = {}
  for (const [key, value] of Object.entries(rawData)) {
    processedData[key] = hashValue(value)
  }

  return processedData
})()
