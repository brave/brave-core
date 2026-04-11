#!/usr/bin/env python3
"""Convert NVIDIA Parakeet NeMo model to candle-compatible
format (safetensors + tokenizer.json + mel_filters.bytes).

Usage:
  # Download the .nemo file first:
  # huggingface-cli download nvidia/parakeet-tdt_ctc-110m \
  #   --include "*.nemo" --local-dir ./

  pip install torch safetensors tokenizers \
    sentencepiece librosa numpy

  python convert_parakeet.py \
    --nemo parakeet-tdt_ctc-110m.nemo \
    --output ./parakeet-110m/

Alternatively, use a HuggingFace model with safetensors:
  python convert_parakeet.py \
    --hf nvidia/parakeet-ctc-0.6b \
    --output ./parakeet-0.6b/
"""

import argparse
import json
import os
import re
import struct
import tarfile
import tempfile
from pathlib import Path

import numpy as np


def generate_mel_filters(
    n_fft=512, n_mels=80, sample_rate=16000
):
    """Generate mel filterbank for Parakeet.

    Uses librosa to compute the mel filterbank matrix.
    Output shape: (n_mels, n_fft//2 + 1).
    """
    import librosa

    filters = librosa.filters.mel(
        sr=sample_rate,
        n_fft=n_fft,
        n_mels=n_mels,
        fmin=0.0,
        fmax=sample_rate / 2.0,
    )
    return filters.astype(np.float32)


def save_mel_filters(filters, output_path):
    """Save mel filters as raw f32 little-endian bytes."""
    with open(output_path, "wb") as f:
        for val in filters.flatten():
            f.write(struct.pack("<f", float(val)))
    print(
        f"  mel_filters.bytes: {filters.shape} "
        f"({os.path.getsize(output_path)} bytes)"
    )


# NeMo -> HuggingFace weight name mapping
NEMO_TO_HF = [
    (
        r"encoder\.pre_encode\.conv\.",
        r"encoder.subsampling.layers.",
    ),
    (
        r"encoder\.pre_encode\.out\.",
        r"encoder.subsampling.linear.",
    ),
    (
        r"encoder\.pos_enc\.",
        r"encoder.encode_positions.",
    ),
    (
        r"encoder\.layers\.(\d+)\.conv\.batch_norm\.",
        r"encoder.layers.\1.conv.norm.",
    ),
    (
        r"decoder\.decoder_layers\.0\.(weight|bias)",
        r"ctc_head.\1",
    ),
    (r"linear_k", r"k_proj"),
    (r"linear_v", r"v_proj"),
    (r"linear_q", r"q_proj"),
    (r"linear_out", r"o_proj"),
    (r"pos_bias_u", r"bias_u"),
    (r"pos_bias_v", r"bias_v"),
    (r"linear_pos", r"relative_k_proj"),
    (r"\.norm_feed_forward\.", r".norm_feed_forward1."),
    (r"\.norm_self_att\.", r".norm_self_att."),
]


def remap_nemo_key(key):
    """Apply NeMo -> HF key remapping."""
    result = key
    for pattern, replacement in NEMO_TO_HF:
        result = re.sub(pattern, replacement, result)
    return result


def convert_nemo(nemo_path, output_dir):
    """Convert a .nemo archive to safetensors."""
    import torch
    from safetensors.torch import save_file

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"Extracting {nemo_path}...")
    with tempfile.TemporaryDirectory() as tmp:
        with tarfile.open(nemo_path, "r") as tar:
            tar.extractall(tmp)

        # Find the checkpoint file
        ckpt_files = list(Path(tmp).rglob("*.ckpt"))
        if not ckpt_files:
            raise FileNotFoundError(
                "No .ckpt found in .nemo archive"
            )
        ckpt_path = ckpt_files[0]
        print(f"  Loading checkpoint: {ckpt_path}")

        state_dict = torch.load(
            ckpt_path,
            map_location="cpu",
            weights_only=False,
        )
        if "state_dict" in state_dict:
            state_dict = state_dict["state_dict"]

        # Remap keys
        new_state_dict = {}
        skipped = []
        for key, tensor in state_dict.items():
            new_key = remap_nemo_key(key)
            # Skip TDT-specific weights (we only
            # need encoder + CTC head)
            if any(
                s in new_key
                for s in [
                    "joint.",
                    "prediction.",
                    "decoder.prediction",
                    "decoder.joint",
                    "num_batches_tracked",
                ]
            ):
                skipped.append(key)
                continue
            new_state_dict[new_key] = tensor.contiguous()

        print(
            f"  Mapped {len(new_state_dict)} tensors, "
            f"skipped {len(skipped)}"
        )

        # Save as safetensors
        st_path = output_dir / "model.safetensors"
        save_file(new_state_dict, str(st_path))
        size_mb = os.path.getsize(st_path) / 1e6
        print(f"  model.safetensors: {size_mb:.1f} MB")

        # Find and convert tokenizer
        spm_files = list(
            Path(tmp).rglob("*.model")
        )
        if spm_files:
            convert_tokenizer(
                spm_files[0], output_dir
            )
        else:
            print(
                "  WARNING: No SentencePiece .model "
                "found in archive"
            )

    # Generate mel filters
    filters = generate_mel_filters(
        n_fft=512, n_mels=80
    )
    save_mel_filters(
        filters, output_dir / "mel_filters.bytes"
    )

    # Write config
    config = {
        "hidden_size": 512,
        "num_heads": 8,
        "num_layers": 17,
        "intermediate_size": 2048,
        "conv_kernel_size": 9,
        "num_mel_bins": 80,
        "vocab_size": 1025,
        "subsampling_channels": 256,
    }
    with open(output_dir / "config.json", "w") as f:
        json.dump(config, f, indent=2)
    print("  config.json written")

    print(f"\nDone! Output in {output_dir}/")


def convert_hf(model_id, output_dir):
    """Download HF model (already safetensors)."""
    from huggingface_hub import snapshot_download

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"Downloading {model_id}...")
    local_dir = snapshot_download(
        model_id,
        allow_patterns=[
            "*.safetensors",
            "config.json",
            "tokenizer.json",
            "tokenizer_config.json",
            "preprocessor_config.json",
        ],
        local_dir=str(output_dir),
    )
    print(f"  Downloaded to {local_dir}")

    # Generate mel filters
    filters = generate_mel_filters(
        n_fft=512, n_mels=80
    )
    save_mel_filters(
        filters, output_dir / "mel_filters.bytes"
    )
    print(f"\nDone! Output in {output_dir}/")


def convert_tokenizer(spm_path, output_dir):
    """Convert SentencePiece .model to
    tokenizer.json format."""
    from tokenizers import SentencePieceBPETokenizer

    print(f"  Converting tokenizer: {spm_path}")

    # Try using sentencepiece directly to extract
    # vocab, then build a tokenizer.json
    import sentencepiece as spm

    sp = spm.SentencePieceProcessor()
    sp.Load(str(spm_path))

    vocab_size = sp.GetPieceSize()
    print(f"  Vocab size: {vocab_size}")

    # Build a minimal tokenizer.json that the
    # tokenizers crate can load.
    # The tokenizers library can load SP models
    # directly in some cases.
    from tokenizers import Tokenizer
    from tokenizers.models import Unigram

    pieces = []
    for i in range(vocab_size):
        piece = sp.IdToPiece(i)
        score = sp.GetScore(i)
        pieces.append((piece, float(score)))

    model = Unigram(pieces, unk_id=0)
    tokenizer = Tokenizer(model)

    # Add Metaspace decoder so tokenizer.decode()
    # replaces ▁ with spaces and joins subwords
    # correctly (e.g. ▁w + ond + ers → "wonders").
    # Without this, raw SentencePiece tokens are
    # returned with ▁ markers intact.
    from tokenizers import decoders
    tokenizer.decoder = decoders.Metaspace()

    # Add the blank token (id = vocab_size)
    tokenizer.add_special_tokens(["<blank>"])

    tok_path = output_dir / "tokenizer.json"
    tokenizer.save(str(tok_path))
    print(
        f"  tokenizer.json: "
        f"{os.path.getsize(tok_path)} bytes"
    )


def main():
    parser = argparse.ArgumentParser(
        description="Convert Parakeet model"
    )
    group = parser.add_mutually_exclusive_group(
        required=True
    )
    group.add_argument(
        "--nemo",
        help="Path to .nemo file",
    )
    group.add_argument(
        "--hf",
        help="HuggingFace model ID",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Output directory",
    )
    args = parser.parse_args()

    if args.nemo:
        convert_nemo(args.nemo, args.output)
    else:
        convert_hf(args.hf, args.output)


if __name__ == "__main__":
    main()
