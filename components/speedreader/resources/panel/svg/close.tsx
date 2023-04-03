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
      d="M3.995 3.995a.567.567 0 0 0 0 .801L7.199 8l-3.207 3.207a.567.567 0 0 0 .801.8L8 8.802l3.204 3.204a.567.567 0 0 0 .801-.801L8.801 8l3.207-3.206a.567.567 0 1 0-.802-.802L8 7.2 4.796 3.995a.567.567 0 0 0-.801 0Z"
      clipRule="evenodd"
    />
  </svg>
)

export default SvgComponent
