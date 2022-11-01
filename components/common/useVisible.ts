import * as React from 'react'

export interface VisibleOptions {
  rootElement?: HTMLElement | null
  rootMargin?: string
  threshold?: number
}

export function useVisible (options: VisibleOptions) {
  const [visible, setVisible] = React.useState(false)
  const [elementRef, setElementRef] = React.useState<HTMLElement | null>(null)

  React.useEffect(() => {
    if (!elementRef) return

    const observer = new IntersectionObserver(([intersectionInfo]) => {
      if (!intersectionInfo.isIntersecting) {
        return
      }
      setVisible(true)
    }, {
      root: options.rootElement,
      rootMargin: options.rootMargin,
      threshold: options.threshold
    })

  observer.observe(elementRef)
    return () => {
      observer.disconnect()
    }
  }, [options.rootElement, elementRef])

  return {
    visible,
    setElementRef
  }
}
