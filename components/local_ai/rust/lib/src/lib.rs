/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

pub mod config;
pub mod embedder;
pub mod error;
pub mod model;

pub use embedder::Gemma3Embedder;
pub use error::EmbedderError;
