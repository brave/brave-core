import * as React from "react"

const SvgComponent = (props: any) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width={16}
    height={16}
    fill="none"
    {...props}
  >
    <circle cx={8} cy={8} r={6} fill="#fff" />
    <path
      fill="#5F5CF1"
      fillRule="evenodd"
      d="M1.333 8a6.667 6.667 0 1 1 13.334 0A6.667 6.667 0 0 1 1.334 8Zm9.136-1.24a.513.513 0 1 0-.835-.597L7.422 9.261l-1.11-1.11a.513.513 0 1 0-.726.724l1.539 1.539a.513.513 0 0 0 .78-.065l2.564-3.59Z"
      clipRule="evenodd"
    />
  </svg>
)

export default SvgComponent
