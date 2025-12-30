/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const transitionStyles = `
  /* Initial page entrance animation */
  .container.entrance-animation {
    opacity: 0;
    animation: entranceFadeScale 0.9s cubic-bezier(0.34, 1.56, 0.64, 1) 0.6s forwards;
  }

  @keyframes entranceFadeScale {
    0% {
      opacity: 0;
      transform: scale(0.85);
    }
    100% {
      opacity: 1;
      transform: scale(1);
    }
  }

  /* Step transition wrapper */
  .step-wrapper {
    display: flex;
    flex-direction: column;
    flex: 1;
    transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1),
                opacity 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  }

  .step-visible {
    transform: translateX(0);
    opacity: 1;
  }

  /* Exiting animations */
  .step-exit-left {
    transform: translateX(-40px);
    opacity: 0;
  }

  .step-exit-right {
    transform: translateX(40px);
    opacity: 0;
  }

  /* Entering animations */
  .step-enter-from-right {
    animation: slideInFromRight 0.3s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  .step-enter-from-left {
    animation: slideInFromLeft 0.3s cubic-bezier(0.4, 0, 0.2, 1) forwards;
  }

  @keyframes slideInFromRight {
    from {
      transform: translateX(40px);
      opacity: 0;
    }
    to {
      transform: translateX(0);
      opacity: 1;
    }
  }

  @keyframes slideInFromLeft {
    from {
      transform: translateX(-40px);
      opacity: 0;
    }
    to {
      transform: translateX(0);
      opacity: 1;
    }
  }

  @keyframes fadeIn {
    from {
      opacity: 0;
      transform: translateY(-8px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }
`

