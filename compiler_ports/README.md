# Compiler Ports for WALI

## Rust

The [`wasm32-wali-linux-musl`](https://doc.rust-lang.org/nightly/rustc/platform-support/wasm32-wali-linux.html) target is upstream in rustc nightly.
Upstream ships no prebuilt `std` for it, so programs are built with `-Zbuild-std`.
Build the runtime, compiler, and libc first, then run:

```shell
make rustc
```
When it finishes, follow the instructions to complete config.

To compile applications:
```shell
cargo +nightly build -Zbuild-std=std,panic_abort --target=wasm32-wali-linux-musl
```
The `build-std` flag can also go in the config as `[unstable] build-std = ["std", "panic_abort"]`.

> To work on the compiler itself, build rustc with the target from your own checkout of the Rust repo:
> 
> ```shell
> make rustc-source RUST_SRC=/path/to/rust
> ```
> 
> This writes the `wasm32-wali-linux-musl` target config into the checkout's `bootstrap.toml` (via `rustc_config.py`, which needs the Python `toml` package), builds stage 1, and links it into `rustup` as a toolchain named `wali`.
> That toolchain includes a prebuilt `std` for the target, so `-Zbuild-std` is not needed:
> 
> ```shell
> cargo +wali build --target=wasm32-wali-linux-musl
> ```
