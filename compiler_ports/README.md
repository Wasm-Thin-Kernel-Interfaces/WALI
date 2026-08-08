# Compiler Ports for WALI

## Rust

> **Note**: Preliminary support for a [`wasm32-wali-linux-musl`](https://doc.rust-lang.org/nightly/rustc/platform-support/wasm32-wali-linux.html) target has been upstreamed to rustc! 
> Support for this target requires several ecosystem components to ratchet up to something usable for long-term stability, and may currently be broken. 
> Use the below out-of-tree build process for rustc if you need a stable target as a proof-of-concept.

We support a custom Rust compiler with a `wasm32-wali-linux-musl` target. 
Existing `cargo` and  `rustup` are required for a successful build.
To build `rustc`, run:

```shell
make rustc
```

This adds a new toolchain to `rustup` named `wali` with the new target.
To compile applications:
```shell
cargo +wali build --target=wasm32-wali-linux-musl
```
