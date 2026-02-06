/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use std::fmt;

#[derive(Debug)]
pub enum EmbedderError {
    CandleError(candle_core::Error),
    TokenizerError(String),
    ModelError(String),
}

impl fmt::Display for EmbedderError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EmbedderError::CandleError(e) => write!(f, "Candle error: {}", e),
            EmbedderError::TokenizerError(e) => {
                write!(f, "Tokenizer error: {}", e)
            }
            EmbedderError::ModelError(e) => write!(f, "Model error: {}", e),
        }
    }
}

impl std::error::Error for EmbedderError {}

impl From<candle_core::Error> for EmbedderError {
    fn from(err: candle_core::Error) -> Self {
        EmbedderError::CandleError(err)
    }
}

impl From<Box<dyn std::error::Error + Send + Sync>> for EmbedderError {
    fn from(err: Box<dyn std::error::Error + Send + Sync>) -> Self {
        EmbedderError::TokenizerError(err.to_string())
    }
}
