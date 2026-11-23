jiff-core
=========
This is a required dependency of `jiff` that provides some of its core
primitives. This includes things like civil datetime algorithms, conversions
to and from Unix timestamps and low level time zone operations. Notable things
not included in this crate include, but are not limited to: a time zone aware
datetime type, formatting, parsing, Serde integration, or calendar durations.

This dependency exists primarily to improve compile times and to share code
between `jiff` and `jiff-static`. It is not intended to provide a stable API
and there are no routines for directly converting between the types in this
crate and in `jiff`. In particular, the semver evolution of `jiff-core` is
independent of `jiff`. In other words, the primary purpose of this crate is to
be an implementation detail of `jiff`. Still, others may find it useful to
depend on a very small library with useful datetime primitives.

### Documentation

https://docs.rs/jiff-core
