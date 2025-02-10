# BetterTLS Test Suite

Generated using the Netflix [bettertls] project.

[bettertls]: https://github.com/Netflix/bettertls

## Test Data

To regenerate vendored test data:

1. Install Go
2. Generate the JSON testdata export:

```bash
GOBIN=$PWD go install github.com/Netflix/bettertls/test-suites/cmd/bettertls@latest
./bettertls export-tests --out ./bettertls.tests.json
```

3. Bzip2 compress it:

```bash
bzip2 ./bettertls.tests.json
```
