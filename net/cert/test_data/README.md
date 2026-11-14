# test_data

`out/pyproto/root_store_pb2.py` is a copy of the Python bindings that a build
generates at `out/<dir>/pyproto/net/cert/root_store_proto_full/`. It exists only
so that `generate_root_cert_list_test.py` can run from `PRESUBMIT.py`, which
must not depend on a build. The build itself always uses freshly generated
bindings.

The `out/` path keeps presubmit's license and lint checks from applying to
generated code. `brave/.ruff.toml` excludes it from formatting.

## Regenerating

The copy can be deleted and regenerated at any time, e.g. when the test fails to
parse `net/data/ssl/chrome_root_store/root_store.textproto` after
`net/cert/root_store.proto` changes upstream.

1. Build the bindings:

   ```sh
   pnpm run build --target=brave/net/cert:root_cert_list
   ```

2. Copy them here, replacing `<dir>` with your build output directory:

   ```sh
   cp ../out/<dir>/pyproto/net/cert/root_store_proto_full/root_store_pb2.py \
     net/cert/test_data/out/pyproto/
   ```

Run both commands from `src/brave`.
