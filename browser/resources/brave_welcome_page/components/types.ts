/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export interface StepActions {
  onNext: () => void
  onBack: () => void
  onSkip: () => void
}

export interface StepState {
  isFirstStep: boolean
  isLastStep: boolean
}

export interface StepContentProps extends StepActions, StepState {}

export interface StepFooterProps extends StepActions, StepState {}

export interface StepDefinition {
  id: string
  Content: React.ComponentType<StepContentProps>
  Footer: React.ComponentType<StepFooterProps>
}

