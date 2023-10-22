import * as React from "react";

export default function Image({ src, ...rest }: React.DetailedHTMLProps<React.ImgHTMLAttributes<HTMLImageElement>, HTMLImageElement>) {
  const ref = React.useRef<HTMLImageElement>(null)
  React.useEffect(() => {
    if (!ref.current) return
    ref.current.setAttribute('style', 'opacity: 0')

    const handler = () => {
      ref.current?.setAttribute('style', '')
    }

    ref.current.addEventListener('load', handler)

    return () => {
      ref.current?.removeEventListener('load', handler)
    }
  }, [src])
  return <img ref={ref} src={src} {...rest} />
}
