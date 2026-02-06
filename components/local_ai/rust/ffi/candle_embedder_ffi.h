/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_RUST_FFI_CANDLE_EMBEDDER_FFI_H_
#define BRAVE_COMPONENTS_LOCAL_AI_RUST_FFI_CANDLE_EMBEDDER_FFI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CCandleEmbedder CCandleEmbedder;

typedef void (*InitCallback)(void* user_data, bool success, const char* data);

typedef void (*EmbedCallback)(void* user_data,
                              const float* embeddings,
                              size_t length);

void candle_embedder_new(const uint8_t* weights,
                         size_t weights_len,
                         const uint8_t* weights_dense1,
                         size_t weights_dense1_len,
                         const uint8_t* weights_dense2,
                         size_t weights_dense2_len,
                         const uint8_t* tokenizer,
                         size_t tokenizer_len,
                         const uint8_t* config,
                         size_t config_len,
                         InitCallback init_cb,
                         void* user_data);

void candle_embedder_embed(CCandleEmbedder* embedder,
                           const char* text,
                           EmbedCallback embed_cb,
                           void* user_data);

void candle_embedder_destroy(CCandleEmbedder* embedder);

#ifdef __cplusplus
}
#endif

#endif  // BRAVE_COMPONENTS_LOCAL_AI_RUST_FFI_CANDLE_EMBEDDER_FFI_H_
