//! Shared helpers for the `wasm32-wali-linux-musl` integration tests.

use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};

static COUNTER: AtomicUsize = AtomicUsize::new(0);

/// A unique scratch directory under `/tmp` for a test to work in.
///
/// The directory is created fresh (removing any leftovers from a previous run);
/// tests are expected to clean up after themselves.
pub fn scratch_dir(name: &str) -> PathBuf {
    let id = COUNTER.fetch_add(1, Ordering::SeqCst);
    let dir = PathBuf::from(format!("/tmp/wali-rust-tests/{name}-{id}"));
    let _ = std::fs::remove_dir_all(&dir);
    std::fs::create_dir_all(&dir).expect("failed to create scratch dir");
    dir
}
