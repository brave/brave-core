#!/bin/sh

set -ex

suffix() {
    name="$1"
    extra="$2"
    cargo metadata | jq -r \
        ".packages.[] | select(.name==\"${name}\") | (\"rust-\" + .name + \"-\" + .version + \"-from-crates-io${extra}\")"
}

generate() {
  file="$1"
  shift
  wit-bindgen rust "$@" --format
}

# ==============================================================================
# WASIp2 bindings

generate_p2() {
  file="$1"

  generate "$@" --std-feature ./crates/wasip2/wit --out-dir crates/wasip2/src

  sed -z -i 's/#\[unsafe(\n    link_section = "\(.*\)"\n)\]/\
#[cfg_attr(feature = "rustc-dep-of-std", unsafe(link_section = "\1-in-libstd"))]\
#[cfg_attr(not(feature = "rustc-dep-of-std"), unsafe(link_section = "\1"))]\
/' $file
}

# Generate the main body of the bindings which includes all imports from the two
# worlds below.
generate_p2 crates/wasip2/src/imports.rs \
  --type-section-suffix $(suffix "wasip2") \
  --generate-all \
  --world wasi:cli/imports

# Generate bindings for the `wasi:cli/command` world specifically, namely the
# macro `export_command`.
#
# Note that `--with` is used to point at the previously generated bindings.
with="wasi:cli/environment@0.2.4=crate::cli::environment"
with="$with,wasi:cli/exit@0.2.4=crate::cli::exit"
with="$with,wasi:cli/stdin@0.2.4=crate::cli::stdin"
with="$with,wasi:cli/stdout@0.2.4=crate::cli::stdout"
with="$with,wasi:cli/stderr@0.2.4=crate::cli::stderr"
with="$with,wasi:cli/terminal-input@0.2.4=crate::cli::terminal_input"
with="$with,wasi:cli/terminal-output@0.2.4=crate::cli::terminal_output"
with="$with,wasi:cli/terminal-stdin@0.2.4=crate::cli::terminal_stdin"
with="$with,wasi:cli/terminal-stdout@0.2.4=crate::cli::terminal_stdout"
with="$with,wasi:cli/terminal-stderr@0.2.4=crate::cli::terminal_stderr"
with="$with,wasi:clocks/monotonic-clock@0.2.4=crate::clocks::monotonic_clock"
with="$with,wasi:clocks/wall-clock@0.2.4=crate::clocks::wall_clock"
with="$with,wasi:filesystem/types@0.2.4=crate::filesystem::types"
with="$with,wasi:filesystem/preopens@0.2.4=crate::filesystem::preopens"
with="$with,wasi:io/error@0.2.4=crate::io::error"
with="$with,wasi:io/poll@0.2.4=crate::io::poll"
with="$with,wasi:io/streams@0.2.4=crate::io::streams"
with="$with,wasi:random/random@0.2.4=crate::random::random"
with="$with,wasi:random/insecure@0.2.4=crate::random::insecure"
with="$with,wasi:random/insecure-seed@0.2.4=crate::random::insecure_seed"
with="$with,wasi:sockets/network@0.2.4=crate::sockets::network"
with="$with,wasi:sockets/instance-network@0.2.4=crate::sockets::instance_network"
with="$with,wasi:sockets/tcp@0.2.4=crate::sockets::tcp"
with="$with,wasi:sockets/tcp-create-socket@0.2.4=crate::sockets::tcp_create_socket"
with="$with,wasi:sockets/udp@0.2.4=crate::sockets::udp"
with="$with,wasi:sockets/udp-create-socket@0.2.4=crate::sockets::udp_create_socket"
with="$with,wasi:sockets/ip-name-lookup@0.2.4=crate::sockets::ip_name_lookup"
generate_p2 crates/wasip2/src/command.rs \
  --world wasi:cli/command \
  --with "$with" \
  --type-section-suffix $(suffix "wasip2" "-command-world") \
  --default-bindings-module '$crate' \
  --pub-export-macro \
  --export-macro-name _export_command

# Same as the `command` world, but for the proxy world.
with="wasi:cli/stdin@0.2.4=crate::cli::stdin"
with="$with,wasi:cli/stdout@0.2.4=crate::cli::stdout"
with="$with,wasi:cli/stderr@0.2.4=crate::cli::stderr"
with="$with,wasi:clocks/monotonic-clock@0.2.4=crate::clocks::monotonic_clock"
with="$with,wasi:clocks/wall-clock@0.2.4=crate::clocks::wall_clock"
with="$with,wasi:io/error@0.2.4=crate::io::error"
with="$with,wasi:io/poll@0.2.4=crate::io::poll"
with="$with,wasi:io/streams@0.2.4=crate::io::streams"
with="$with,wasi:random/random@0.2.4=crate::random::random"
generate_p2 crates/wasip2/src/proxy.rs \
  --world wasi:http/proxy \
  --with "$with" \
  --type-section-suffix $(suffix "wasip2" "-proxy-world") \
  --default-bindings-module '$crate' \
  --pub-export-macro \
  --export-macro-name _export_proxy

# ==============================================================================
# WASIp3 bindings

generate_p3() {
  generate "$@" ./crates/wasip3/wit --out-dir crates/wasip3/src
}

generate_p3 crates/wasip3/src/imports.rs \
  --type-section-suffix $(suffix "wasip3") \
  --generate-all \
  --world wasi:cli/imports

with="wasi:cli/environment@0.3.0-rc-2025-08-15=crate::cli::environment"
with="$with,wasi:cli/exit@0.3.0-rc-2025-08-15=crate::cli::exit"
with="$with,wasi:cli/stdin@0.3.0-rc-2025-08-15=crate::cli::stdin"
with="$with,wasi:cli/stdout@0.3.0-rc-2025-08-15=crate::cli::stdout"
with="$with,wasi:cli/stderr@0.3.0-rc-2025-08-15=crate::cli::stderr"
with="$with,wasi:cli/terminal-input@0.3.0-rc-2025-08-15=crate::cli::terminal_input"
with="$with,wasi:cli/terminal-output@0.3.0-rc-2025-08-15=crate::cli::terminal_output"
with="$with,wasi:cli/terminal-stdin@0.3.0-rc-2025-08-15=crate::cli::terminal_stdin"
with="$with,wasi:cli/terminal-stdout@0.3.0-rc-2025-08-15=crate::cli::terminal_stdout"
with="$with,wasi:cli/terminal-stderr@0.3.0-rc-2025-08-15=crate::cli::terminal_stderr"
with="$with,wasi:clocks/monotonic-clock@0.3.0-rc-2025-08-15=crate::clocks::monotonic_clock"
with="$with,wasi:clocks/wall-clock@0.3.0-rc-2025-08-15=crate::clocks::wall_clock"
with="$with,wasi:filesystem/types@0.3.0-rc-2025-08-15=crate::filesystem::types"
with="$with,wasi:filesystem/preopens@0.3.0-rc-2025-08-15=crate::filesystem::preopens"
with="$with,wasi:random/random@0.3.0-rc-2025-08-15=crate::random::random"
with="$with,wasi:random/insecure@0.3.0-rc-2025-08-15=crate::random::insecure"
with="$with,wasi:random/insecure-seed@0.3.0-rc-2025-08-15=crate::random::insecure_seed"
with="$with,wasi:sockets/types@0.3.0-rc-2025-08-15=crate::sockets::types"
with="$with,wasi:sockets/ip-name-lookup@0.3.0-rc-2025-08-15=crate::sockets::ip_name_lookup"
generate_p3 crates/wasip3/src/command.rs \
  --world wasi:cli/command \
  --with "$with" \
  --type-section-suffix $(suffix "wasip3" "-command-world") \
  --default-bindings-module '$crate' \
  --pub-export-macro \
  --async 'wasi:cli/run@0.3.0-rc-2025-08-15#run' \
  --export-macro-name _export_command

with="wasi:cli/stdin@0.3.0-rc-2025-08-15=crate::cli::stdin"
with="$with,wasi:cli/stdout@0.3.0-rc-2025-08-15=crate::cli::stdout"
with="$with,wasi:cli/stderr@0.3.0-rc-2025-08-15=crate::cli::stderr"
with="$with,wasi:clocks/monotonic-clock@0.3.0-rc-2025-08-15=crate::clocks::monotonic_clock"
with="$with,wasi:clocks/wall-clock@0.3.0-rc-2025-08-15=crate::clocks::wall_clock"
with="$with,wasi:random/random@0.3.0-rc-2025-08-15=crate::random::random"
generate_p3 crates/wasip3/src/proxy.rs \
  --world wasi:http/proxy \
  --with "$with" \
  --type-section-suffix $(suffix "wasip3" "-proxy-world") \
  --default-bindings-module '$crate' \
  --pub-export-macro \
  --export-macro-name _export_proxy
