import * as React from 'react'
import { LibContext } from '../context/lib.context'

export const useLib = () => {
  return React.useContext(LibContext)
}
