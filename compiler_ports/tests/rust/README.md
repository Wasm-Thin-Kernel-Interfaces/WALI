# Rust Target Tests

Dependency-free `cargo test` integration tests for the `wasm32-wali-linux-musl`
target. Requires the `wali` toolchain (`make rustc`) and a built runtime
(`make iwasm`).

```shell
make -C compiler_ports/tests/rust test     # or: cargo +wali test
```

Cargo cross-compiles each test binary and runs it under the `runner` set in
[.cargo/config.toml](.cargo/config.toml) — the repo-root `iwasm` symlink.
Override it with `CARGO_TARGET_WASM32_WALI_LINUX_MUSL_RUNNER`.
