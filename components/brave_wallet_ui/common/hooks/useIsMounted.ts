import * as React from 'react'

export function useIsMounted () {
  const isMounted = React.useRef(true)
  React.useEffect(() => {
    // cleanup mechanism for useEffect
    // will be called during component unmount
    return () => {
      isMounted.current = false
    }
  }, [])

  return isMounted.current
}

export default useIsMounted
