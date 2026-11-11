# Installer test data

## test_patch.packed.7z

A compressed archive holding a differential update patch instead of chrome.7z,
as consumed by `SetupUnpackArchiveDeltaTest` in
`brave/chromium_src/chrome/installer/setup/unpack_archive_unittest.cc`.

To regenerate:

```
copy ..\..\..\..\chrome\test\data\installer\zucchini_archive.diff chrome_patch.diff
..\..\..\..\third_party\lzma_sdk\bin\host_platform\7za.exe a -m0=LZMA test_patch.packed.7z chrome_patch.diff
```

`zucchini_archive.diff` is a Zucchini patch that transforms
`chrome/test/data/installer/archive1.7z` into `archive2.7z`.
