/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Chunked GGUF loader for candle_nemotron. The algorithm is identical
// to candle_parakeet's loader -- nothing here is Parakeet- or
// Nemotron-specific. See parakeet's builder.rs for the prose-level
// explanation of the three-phase load.

use std::collections::HashMap;

use candle_core::quantized::ggml_file::qtensor_from_ggml;
use candle_core::quantized::gguf_file;
use candle_core::{DType, Device, Result, Tensor};
use candle_nn::VarBuilder;

use super::{Nemotron, NemotronConfig};

pub struct ModelBuilder {
    content: gguf_file::Content,
    tensors: HashMap<String, Tensor>,
    bytes_received: usize,
    spill: Vec<u8>,
    spill_offset: usize,
}

impl ModelBuilder {
    pub fn new(content: gguf_file::Content) -> Self {
        Self {
            content,
            tensors: HashMap::new(),
            bytes_received: 0,
            spill: Vec::new(),
            spill_offset: 0,
        }
    }

    pub fn load_chunk(&mut self, chunk: &[u8]) -> Result<()> {
        let combined: Vec<u8>;
        let (view, view_start) = if self.spill.is_empty() {
            (chunk, self.bytes_received)
        } else {
            combined = [self.spill.as_slice(), chunk].concat();
            let start = self.spill_offset;
            self.spill.clear();
            (combined.as_slice(), start)
        };
        let view_end = view_start + view.len();

        for (name, info) in &self.content.tensor_infos {
            if self.tensors.contains_key(name) {
                continue;
            }
            let t_start = info.offset as usize;
            let elems = info.shape.elem_count();
            let bs = info.ggml_dtype.block_size();
            let t_size = elems / bs * info.ggml_dtype.type_size();
            let t_end = t_start + t_size;

            if t_start >= view_end || t_end <= view_start {
                continue;
            }

            if t_end > view_end {
                let local = t_start.saturating_sub(view_start);
                self.spill = view[local..].to_vec();
                self.spill_offset = t_start;
                continue;
            }

            let local = t_start - view_start;
            let slice = &view[local..local + t_size];
            let qt = qtensor_from_ggml(
                info.ggml_dtype,
                slice,
                info.shape.dims().to_vec(),
                &Device::Cpu,
            )?;
            let t = qt.dequantize(&Device::Cpu)?;
            self.tensors.insert(name.clone(), t);
        }

        self.bytes_received += chunk.len();
        Ok(())
    }

    pub fn build(self, config: &NemotronConfig) -> Result<Nemotron> {
        let vb = VarBuilder::from_tensors(
            self.tensors,
            DType::F32,
            &Device::Cpu,
        );
        Nemotron::new(config, vb)
    }
}
