// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as THREE from 'three'

export type OrbState = 'idle' | 'listening' | 'thinking' | 'speaking'

interface VoiceOrbProps {
  state: OrbState
  audioLevel?: number
  className?: string
  onClick?: () => void
}

interface LayerConfig {
  color: number
  opacity: number
  scale: number
  rotationSpeed: { x: number; y: number; z: number }
}

interface StateConfig {
  layers: LayerConfig[]
  audioLevel: number
  audioFrequency: number
  timeSpeed: number
  pulsate: boolean
  pulsateMode?: 'audio-reactive' | 'thinking' | 'cadence'
  pulsateMin?: number
  pulsateMax?: number
  chromaticAberration: number
  description: string
}

// State configurations - Layered sphere theme with colors that work in light/dark mode
const states: Record<OrbState, StateConfig> = {
  idle: {
    layers: [
      {
        color: 0x434FCF,
        opacity: 0.2,
        scale: 1.0,
        rotationSpeed: { x: 0.001, y: 0.002, z: 0 }
      },
      {
        color: 0x434FCF,
        opacity: 0.2,
        scale: 0.85,
        rotationSpeed: { x: -0.002, y: 0.003, z: 0.001 }
      },
      {
        color: 0x434FCF,
        opacity: 0.4,
        scale: 0.70,
        rotationSpeed: { x: 0.003, y: -0.002, z: -0.001 }
      }
    ],
    audioLevel: 0.15,
    audioFrequency: 0.2,
    timeSpeed: 0.015,
    pulsate: false,
    chromaticAberration: 0.8,
    description: 'Calm and ready'
  },
  listening: {
    layers: [
      {
        color: 0x434FCF,
        opacity: 0.2,
        scale: 1.0,
        rotationSpeed: { x: 0.002, y: 0.004, z: 0 }
      },
      {
        color: 0x434FCF,
        opacity: 0.2,
        scale: 0.85,
        rotationSpeed: { x: -0.003, y: 0.005, z: 0.002 }
      },
      {
        color: 0x434FCF,
        opacity: 0.4,
        scale: 0.70,
        rotationSpeed: { x: 0.004, y: -0.003, z: -0.001 }
      }
    ],
    audioLevel: 0.6,
    audioFrequency: 0.7,
    timeSpeed: 0.022,
    pulsate: true,
    pulsateMode: 'audio-reactive',
    pulsateMin: 0.0,
    pulsateMax: 0.15,
    chromaticAberration: 1.2,
    description: 'Actively listening'
  },
  thinking: {
    layers: [
      {
        color: 0x8747F7,
        opacity: 0.2,
        scale: 0.85,
        rotationSpeed: { x: 0.003, y: 0.003, z: 0 }
      },
      {
        color: 0x8747F7,
        opacity: 0.2,
        scale: 0.72,
        rotationSpeed: { x: -0.004, y: 0.004, z: 0.002 }
      },
      {
        color: 0x8747F7,
        opacity: 0.4,
        scale: 0.60,
        rotationSpeed: { x: 0.005, y: -0.004, z: -0.002 }
      }
    ],
    audioLevel: 0.45,
    audioFrequency: 0.5,
    timeSpeed: 0.02,
    pulsate: true,
    pulsateMode: 'thinking',
    pulsateMin: 0.0,
    pulsateMax: 0.15,
    chromaticAberration: 0.8,
    description: 'Processing...'
  },
  speaking: {
    layers: [
      {
        color: 0xFF1893,
        opacity: 0.2,
        scale: 1.0,
        rotationSpeed: { x: 0.004, y: 0.005, z: 0 }
      },
      {
        color: 0xFF1893,
        opacity: 0.2,
        scale: 0.85,
        rotationSpeed: { x: -0.005, y: 0.006, z: 0.003 }
      },
      {
        color: 0xFF1893,
        opacity: 0.4,
        scale: 0.70,
        rotationSpeed: { x: 0.006, y: -0.005, z: -0.002 }
      }
    ],
    audioLevel: 0.8,
    audioFrequency: 0.9,
    timeSpeed: 0.027,
    pulsate: true,
    pulsateMode: 'cadence',
    pulsateMin: 0.05,
    pulsateMax: 0.22,
    chromaticAberration: 1.5,
    description: 'Speaking...'
  }
}

// Vertex shader for layered spheres with distortion
const vertexShader = `
  varying vec3 vNormal;
  varying vec3 vPosition;
  varying vec2 vUv;

  uniform float time;
  uniform float audioLevel;
  uniform float layerOffset;

  // Simple noise function for organic distortion
  vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
  vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
  vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }
  vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

  float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    i = mod289(i);
    vec4 p = permute(permute(permute(
      i.z + vec4(0.0, i1.z, i2.z, 1.0))
      + i.y + vec4(0.0, i1.y, i2.y, 1.0))
      + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    float n_ = 0.142857142857;
    vec3 ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = x_ *ns.x + ns.yyyy;
    vec4 y = y_ *ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0)*2.0 + 1.0;
    vec4 s1 = floor(b1)*2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2,p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
  }

  void main() {
    vUv = uv;
    vNormal = normalize(normalMatrix * normal);

    vec3 pos = position;

    // Wave distortion - flowing patterns
    float wave1 = sin(pos.y * 2.5 + time * 1.5 + layerOffset) * cos(pos.x * 2.0 - time * 1.2);
    float wave2 = sin(pos.x * 3.0 - time * 1.8 + layerOffset) * cos(pos.z * 2.5 + time * 1.5);
    float wave3 = sin(pos.z * 2.8 + time * 1.6 + layerOffset) * cos(pos.y * 2.3 - time * 1.3);

    // Noise-based organic distortion
    float noise1 = snoise(pos * 1.2 + time * 0.3 + layerOffset);
    float noise2 = snoise(pos * 2.0 - time * 0.2 + layerOffset * 0.5);

    // Combine distortions - reduced intensity
    float distortion = (wave1 + wave2 + wave3) * 0.008;
    distortion += (noise1 * 0.008 + noise2 * 0.007);

    // Audio reactivity
    distortion *= (0.3 + audioLevel * 0.6);

    // Apply distortion along normal
    pos = pos + normal * distortion;

    vPosition = pos;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
  }
`

// Fragment shader with holographic rainbow effect
const fragmentShader = `
  varying vec3 vNormal;
  varying vec3 vPosition;
  varying vec2 vUv;

  uniform vec3 sphereColor;
  uniform float opacity;
  uniform float time;
  uniform float chromaticAberration;

  // RGB to HSV conversion
  vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
  }

  // HSV to RGB conversion
  vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
  }

  void main() {
    // Calculate fresnel-like effect based on view angle
    vec3 viewDirection = normalize(cameraPosition - vPosition);
    float fresnel = pow(1.0 - abs(dot(viewDirection, normalize(vNormal))), 2.0);

    // Holographic rainbow effect based on surface normal and view angle
    vec3 normalWorld = normalize(vNormal);

    // Create rainbow gradient based on normal direction and position
    float rainbowShift = normalWorld.x * 0.5 + normalWorld.y * 0.2 + normalWorld.z * 0.1;
    rainbowShift += sin(vPosition.x * 5.0 + time * 0.5) * 0.01;
    rainbowShift += cos(vPosition.y * 4.0 - time * 0.3) * 0.01;
    rainbowShift = fract(rainbowShift);

    // Generate holographic rainbow colors
    vec3 rainbow = hsv2rgb(vec3(rainbowShift, 0.8, 1.0));

    // Convert base color to HSV
    vec3 hsv = rgb2hsv(sphereColor);

    // Create chromatic aberration by shifting hue based on position
    float aberrationAmount = chromaticAberration * fresnel;

    // Shift red channel
    vec3 hsvR = hsv;
    hsvR.x = fract(hsv.x + aberrationAmount * 0.15);
    vec3 colorR = hsv2rgb(hsvR);

    // Keep green as base
    vec3 colorG = sphereColor;

    // Shift blue channel opposite direction
    vec3 hsvB = hsv;
    hsvB.x = fract(hsv.x - aberrationAmount * 0.15);
    vec3 colorB = hsv2rgb(hsvB);

    // Mix channels for chromatic aberration effect
    vec3 color = vec3(colorR.r, colorG.g, colorB.b);

    // Blend in holographic rainbow effect, stronger at edges (fresnel)
    float holographicIntensity = fresnel * 0.6 + 0.2;
    color = mix(color, rainbow, holographicIntensity * 0.6);

    // Add edge emphasis where aberration is strongest
    color += fresnel * chromaticAberration * 0.15;

    // Add subtle brightness variation based on position
    float brightness = 1.0 + sin(vPosition.x * 3.0 + time) * 0.1;
    brightness += sin(vPosition.y * 2.5 - time * 0.8) * 0.1;

    // Add extra shimmer for holographic effect
    float shimmer = sin(vPosition.x * 8.0 + vPosition.y * 6.0 + time * 2.0) * 0.04 + 0.96;
    brightness *= shimmer;

    color *= brightness;

    gl_FragColor = vec4(color, opacity);
  }
`

export const VoiceOrb: React.FC<VoiceOrbProps> = ({
  state,
  audioLevel = 0,
  className,
  onClick
}) => {
  const canvasRef = React.useRef<HTMLCanvasElement>(null)
  const sceneRef = React.useRef<THREE.Scene | null>(null)
  const cameraRef = React.useRef<THREE.PerspectiveCamera | null>(null)
  const rendererRef = React.useRef<THREE.WebGLRenderer | null>(null)
  const orbLayersRef = React.useRef<THREE.Mesh[]>([])
  const animationFrameRef = React.useRef<number | null>(null)

  // Animation state
  const targetScaleRef = React.useRef(1.0)
  const currentScaleRef = React.useRef(1.0)
  const targetChromaticAberrationRef = React.useRef(0.8)
  const currentChromaticAberrationRef = React.useRef(0.8)
  const thinkingTimeRef = React.useRef(0)

  // Initialize Three.js scene
  React.useEffect(() => {
    if (!canvasRef.current) return

    // Scene
    const scene = new THREE.Scene()
    sceneRef.current = scene

    // Camera
    const camera = new THREE.PerspectiveCamera(75, 1, 0.1, 1000)
    camera.position.z = 2.5  // Move camera closer to make orb fill more of the canvas
    cameraRef.current = camera

    // Renderer
    const renderer = new THREE.WebGLRenderer({
      canvas: canvasRef.current,
      antialias: true,
      alpha: true
    })
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
    rendererRef.current = renderer

    // Create layered concentric spheres
    const initialState = states.idle
    const orbLayers: THREE.Mesh[] = []

    initialState.layers.forEach((layerConfig, index) => {
      const geometry = new THREE.SphereGeometry(layerConfig.scale, 80, 80)

      const material = new THREE.ShaderMaterial({
        vertexShader,
        fragmentShader,
        uniforms: {
          time: { value: 0 },
          audioLevel: { value: 0 },
          layerOffset: { value: index * 2.0 },
          sphereColor: { value: new THREE.Color(layerConfig.color) },
          opacity: { value: layerConfig.opacity },
          chromaticAberration: { value: initialState.chromaticAberration },
          cameraPosition: { value: camera.position }
        },
        transparent: true,
        side: THREE.DoubleSide,
        blending: THREE.NormalBlending,
        depthWrite: false
      })

      const sphere = new THREE.Mesh(geometry, material)
      sphere.userData = {
        baseScale: layerConfig.scale,
        rotationSpeed: layerConfig.rotationSpeed,
        layerIndex: index,
        targetColor: new THREE.Color(layerConfig.color),
        currentColor: new THREE.Color(layerConfig.color),
        targetOpacity: layerConfig.opacity,
        currentOpacity: layerConfig.opacity
      }

      scene.add(sphere)
      orbLayers.push(sphere)
    })

    orbLayersRef.current = orbLayers

    // Lighting
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.4)
    scene.add(ambientLight)

    const pointLight1 = new THREE.PointLight(0x667eea, 0.6, 100)
    pointLight1.position.set(5, 5, 5)
    scene.add(pointLight1)

    const pointLight2 = new THREE.PointLight(0x764ba2, 0.4, 100)
    pointLight2.position.set(-5, -5, 5)
    scene.add(pointLight2)

    // Handle resize
    const handleResize = () => {
      if (!canvasRef.current || !renderer || !camera) return

      const width = canvasRef.current.clientWidth
      const height = canvasRef.current.clientHeight

      camera.aspect = width / height
      camera.updateProjectionMatrix()
      renderer.setSize(width, height)
    }

    const resizeObserver = new ResizeObserver(handleResize)
    resizeObserver.observe(canvasRef.current)
    handleResize()

    // Cleanup
    return () => {
      resizeObserver.disconnect()
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current)
      }
      orbLayers.forEach(layer => {
        layer.geometry.dispose()
        if (layer.material instanceof THREE.ShaderMaterial) {
          layer.material.dispose()
        }
      })
      renderer.dispose()
    }
  }, [])

  // Update state
  React.useEffect(() => {
    const stateConfig = states[state]
    if (!stateConfig) return

    // Set target chromatic aberration
    targetChromaticAberrationRef.current = stateConfig.chromaticAberration

    // Update each layer's targets
    orbLayersRef.current.forEach((layer, index) => {
      const layerConfig = stateConfig.layers[index]
      if (layerConfig) {
        layer.userData.targetColor = new THREE.Color(layerConfig.color)
        layer.userData.targetOpacity = layerConfig.opacity
        layer.userData.rotationSpeed = layerConfig.rotationSpeed
        layer.userData.baseScale = layerConfig.scale
      }
    })
  }, [state])

  // Animation loop
  React.useEffect(() => {
    const animate = () => {
      if (!sceneRef.current || !cameraRef.current || !rendererRef.current) return

      const stateConfig = states[state]
      let currentAudioLevel = audioLevel

      // Use predefined values if no audio level provided
      if (currentAudioLevel === 0 && !stateConfig.pulsate) {
        currentAudioLevel = stateConfig.audioLevel
      }

      // Handle pulsation
      if (stateConfig.pulsate) {
        if (stateConfig.pulsateMode === 'audio-reactive') {
          const volumeScale = Math.min(1.0, Math.sqrt(currentAudioLevel * 3.5))
          targetScaleRef.current = 1.0 + stateConfig.pulsateMin! +
            (volumeScale * (stateConfig.pulsateMax! - stateConfig.pulsateMin!))
        } else if (stateConfig.pulsateMode === 'thinking') {
          thinkingTimeRef.current += 0.016 // ~60fps
          const thinkingPulse = (Math.sin(thinkingTimeRef.current * 1.5) + 1.0) / 2.0
          targetScaleRef.current = 1.0 + stateConfig.pulsateMin! +
            (thinkingPulse * (stateConfig.pulsateMax! - stateConfig.pulsateMin!))
        } else if (stateConfig.pulsateMode === 'cadence') {
          const cadenceIntensity = currentAudioLevel > 0 ? currentAudioLevel : 0.5
          targetScaleRef.current = 1.0 + stateConfig.pulsateMin! +
            (cadenceIntensity * (stateConfig.pulsateMax! - stateConfig.pulsateMin!))
        }

        const smoothing = currentAudioLevel > 0 ? 0.35 : 0.15
        currentScaleRef.current += (targetScaleRef.current - currentScaleRef.current) * smoothing
      } else {
        targetScaleRef.current = 1.0
        currentScaleRef.current += (targetScaleRef.current - currentScaleRef.current) * 0.1
      }

      // Smoothly interpolate chromatic aberration
      currentChromaticAberrationRef.current +=
        (targetChromaticAberrationRef.current - currentChromaticAberrationRef.current) * 0.08

      // Update each layer
      orbLayersRef.current.forEach((layer) => {
        // Smooth color transition
        if (layer.userData.currentColor && layer.userData.targetColor) {
          layer.userData.currentColor.lerp(layer.userData.targetColor, 0.08)
          if (layer.material instanceof THREE.ShaderMaterial) {
            layer.material.uniforms.sphereColor.value.copy(layer.userData.currentColor)
          }
        }

        // Smooth opacity transition
        if (layer.userData.currentOpacity !== undefined &&
            layer.userData.targetOpacity !== undefined) {
          layer.userData.currentOpacity +=
            (layer.userData.targetOpacity - layer.userData.currentOpacity) * 0.08
          if (layer.material instanceof THREE.ShaderMaterial) {
            layer.material.uniforms.opacity.value = layer.userData.currentOpacity
          }
        }

        // Update shader uniforms
        if (layer.material instanceof THREE.ShaderMaterial) {
          layer.material.uniforms.time.value += stateConfig.timeSpeed
          layer.material.uniforms.audioLevel.value = currentAudioLevel

          let aberrationValue = currentChromaticAberrationRef.current
          if (currentAudioLevel > 0) {
            aberrationValue += currentAudioLevel * 0.3
          }
          layer.material.uniforms.chromaticAberration.value = aberrationValue
        }

        // Apply rotation
        layer.rotation.x += layer.userData.rotationSpeed.x
        layer.rotation.y += layer.userData.rotationSpeed.y
        layer.rotation.z += layer.userData.rotationSpeed.z

        // Apply pulsating scale
        const layerScale = layer.userData.baseScale * currentScaleRef.current
        layer.scale.set(layerScale, layerScale, layerScale)
      })

      rendererRef.current.render(sceneRef.current, cameraRef.current)
      animationFrameRef.current = requestAnimationFrame(animate)
    }

    animate()

    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current)
      }
    }
  }, [state, audioLevel])

  return (
    <canvas
      ref={canvasRef}
      className={className}
      onClick={onClick}
      style={{ width: '100%', height: '100%', cursor: onClick ? 'pointer' : 'default' }}
    />
  )
}
