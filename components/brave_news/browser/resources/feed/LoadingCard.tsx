import styled from "styled-components";
import Card from "./Card";
import ProgressRing from '@brave/leo/react/progressRing'
import * as React from "react";

const Container = styled(Card)`
  display: flex;
  align-items: center;
  justify-content: center;
`

export default function LoadingCard() {
  return <Container>
    <ProgressRing />
  </Container>
}
