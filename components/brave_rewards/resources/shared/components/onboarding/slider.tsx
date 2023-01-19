/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import styled from 'styled-components'

const style = {
  root: styled.div`
    position: relative;

    .slider-bar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      overflow: visible;
      height: var(--slider-bar-height);
    }

    .slider-handle {
      position: absolute;
      height: var(--slider-handle-height);
      width: var(--slider-handle-width);
      top: calc(
        (var(--slider-bar-height) - var(--slider-handle-height)) / 2);
      left: calc(
        var(--slider-handle-position) - (var(--slider-handle-width) / 2));
    }
  `
}

interface Props {
  value: number
  options: number[]
  autoFocus?: boolean
  onChange: (value: number) => void
}

export function Slider (props: Props) {
  const sliderRef = React.useRef<HTMLDivElement | null>(null)

  React.useEffect(() => {
    const sliderElem = sliderRef.current
    if (!sliderElem) {
      return
    }
    const selected = sliderElem.querySelector<HTMLElement>('.slider-selected')
    if (selected) {
      const x = selected.offsetLeft + (selected.offsetWidth / 2)
      sliderElem.style.setProperty('--slider-handle-position', `${x}px`)
    }
  }, [props.value])

  const offsetToValue = (offset: number, range: number) => {
    const slots = props.options.length
    const slot = Math.round(offset / range * (slots - 1))
    const index = Math.min(Math.max(0, slot), slots - 1)
    return props.options[index]
  }

  const updateSliderPosition = (offset: number) => {
    const sliderElem = sliderRef.current
    if (sliderElem) {
      const origin = sliderElem.getBoundingClientRect().left
      props.onChange(offsetToValue(offset - origin, sliderElem.offsetWidth))
    }
  }

  const onSliderBarClick = (event: React.MouseEvent<HTMLElement>) => {
    updateSliderPosition(event.pageX)
  }

  const onSliderHandleKeyDown = (event: React.KeyboardEvent) => {
    const currentIndex = props.options.indexOf(props.value)
    if (currentIndex === -1) {
      return
    }
    switch (event.key) {
      case 'ArrowLeft':
      case 'ArrowDown': {
        if (currentIndex > 0) {
          props.onChange(props.options[currentIndex - 1])
        }
        break
      }
      case 'ArrowRight':
      case 'ArrowUp': {
        if (currentIndex < props.options.length - 1) {
          props.onChange(props.options[currentIndex + 1])
        }
        break
      }
    }
  }

  const onDrag = (event: MouseEvent) => {
    requestAnimationFrame(() => updateSliderPosition(event.pageX))
  }

  const startDrag = () => {
    document.addEventListener('mouseup', stopDrag)
    document.addEventListener('mousemove', onDrag)
  }

  const stopDrag = () => {
    document.removeEventListener('mouseup', stopDrag)
    document.removeEventListener('mousemove', onDrag)
  }

  return (
    <style.root className='slider' ref={sliderRef}>
      <div className='slider-bar' onClick={onSliderBarClick}>
        {
          props.options.map((value) => {
            const classNames = ['slider-marker']
            if (value === props.value) {
              classNames.push('slider-selected')
            }
            return <div key={value} className={classNames.join(' ')} />
          })
        }
      </div>
      <button
        className='slider-handle'
        autoFocus={props.autoFocus}
        onMouseDown={startDrag}
        onKeyDown={onSliderHandleKeyDown}
      />
    </style.root>
  )
}
