import * as React from "react"

const SvgComponent = (props: any) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width={16}
    height={16}
    fill="none"
    {...props}
  >
    <path
      fill="currentColor"
      fillRule="evenodd"
      d="M13.556 14.222h-1.112a1.112 1.112 0 0 1-1.11-1.11V9.777c0-.612.498-1.11 1.11-1.11h1.111v-.001A5.562 5.562 0 0 0 8 3.11a5.562 5.562 0 0 0-5.556 5.556h1.111c.613 0 1.112.499 1.112 1.111v3.333c0 .613-.499 1.111-1.112 1.111h-1.11a1.112 1.112 0 0 1-1.112-1.11V8.666A6.674 6.674 0 0 1 8 2a6.674 6.674 0 0 1 6.667 6.667v4.444c0 .613-.499 1.111-1.111 1.111Zm-11.112-1.11h1.11l.001-3.334h-1.11v3.333Zm10-3.334v3.333h1.11l.002-3.333h-1.112Z"
      clipRule="evenodd"
    />
  </svg>
)

export default SvgComponent
