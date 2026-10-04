## Release process

1. Update the version `[workspace.package]` in `Cargo.toml`.
2. Update the version of `parity-scale-codec-derive` dependency for `parity-scale-codec` in `Cargo.toml`
   (`parity-scale-codec-derive = { .., version = "=x.y.z", ...}`).
3. Test the new release on `polkadot-sdk`, ensure `parity-scale-codec-derive` is also updated when testing.
4. Update the `CHANGELOG.md`.
5. Merge all these changes to master.
6. Add and push a git tag with the version number: `git tag "vX.Y.Z"; git push --tags`.
7. Publish on `crates.io` the package `parity-scale-codec-derive` and then `parity-scale-codec`.
