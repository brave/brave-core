import * as React from 'react'

import * as S from './style'

function ZoomControl () {
  return (
    <S.Box>
      <S.ButtonLeft>
        <svg width="24" height="24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M19.01 11H4.99c-.533 0-.99.461-.99 1 0 .539.457 1 .99 1h14.096c.457-.077.914-.461.914-1 0-.539-.457-1-.99-1Z" fill="currentColor"/></svg>
      </S.ButtonLeft>
      <span>100%</span>
      <S.ButtonRight>
        <svg width="16" height="16" fill="none" xmlns="http://www.w3.org/2000/svg"><path fill-rule="evenodd" clip-rule="evenodd" d="M7 7V.99C7 .457 7.462 0 8 0s.923.457 1 .914V7h6.01c.533 0 .99.462.99 1s-.457.923-.914 1H9v6.01c0 .533-.462.99-1 .99s-1-.457-1-.99V9H.99C.457 9 0 8.538 0 8s.457-1 .99-1H7Z" fill="currentColor"/></svg>
      </S.ButtonRight>
    </S.Box>
  )
}

export default ZoomControl
