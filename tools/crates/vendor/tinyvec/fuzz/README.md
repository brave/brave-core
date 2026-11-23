## Quickstart

```console
> cargo install honggfuzz
> cargo hfuzz run arrayish
```

When a crash is found:
```console
> cargo hfuzz run-debug arrayish hfuzz_workspace/arrayish/*.fuzz
```
