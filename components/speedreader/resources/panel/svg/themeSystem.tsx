import * as React from "react"

const SvgComponent = (props: any) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width={20}
    height={20}
    fill="none"
    {...props}
  >
    <g clipPath="url(#a)">
      <circle cx={10} cy={10} r={10} fill="#fff" />
      <path fill="#000" d="M10 14a4 4 0 0 0 0-8v8Z" />
      <path
        fill="#000"
        fillRule="evenodd"
        d="M10 0C4.477 0 0 4.477 0 10s4.477 10 10 10 10-4.477 10-10S15.523 0 10 0Zm0 1v5a4 4 0 1 0 0 8v5c5.5 0 9-4.5 9-9 0-5-3.5-9-9-9Z"
        clipRule="evenodd"
      />
    </g>
    <rect width={19} height={19} x={0.5} y={0.5} stroke="#E6EBEF" rx={9.5} />
    <defs>
      <clipPath id="a">
        <rect width={20} height={20} fill="#fff" rx={10} />
      </clipPath>
    </defs>
  </svg>
)

export default SvgComponent
