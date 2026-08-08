//! Filesystem syscalls reached through `std::fs` / `std::io`.

use std::fs;
use std::io::{BufRead, BufReader, Read, Seek, SeekFrom, Write};
use wali_rust_tests::scratch_dir;

#[test]
fn write_stat_read_remove() {
    let dir = scratch_dir("fs-basic");
    let path = dir.join("payload.txt");

    let mut f = fs::File::create(&path).unwrap();
    f.write_all(b"wali-rust-payload\n").unwrap();
    f.sync_all().unwrap();
    drop(f);

    let meta = fs::metadata(&path).unwrap();
    assert_eq!(meta.len(), 18);
    assert!(meta.is_file());

    let mut f = fs::File::open(&path).unwrap();
    f.seek(SeekFrom::Start(5)).unwrap();
    let mut tail = String::new();
    f.read_to_string(&mut tail).unwrap();
    assert_eq!(tail, "rust-payload\n");

    fs::remove_file(&path).unwrap();
    assert!(fs::metadata(&path).is_err());
    fs::remove_dir_all(&dir).unwrap();
}

#[test]
fn directory_listing_and_rename() {
    let dir = scratch_dir("fs-dir");
    fs::write(dir.join("a.txt"), "a").unwrap();
    fs::write(dir.join("b.txt"), "b").unwrap();
    fs::create_dir(dir.join("nested")).unwrap();

    let mut names: Vec<String> = fs::read_dir(&dir)
        .unwrap()
        .map(|e| e.unwrap().file_name().into_string().unwrap())
        .collect();
    names.sort();
    assert_eq!(names, ["a.txt", "b.txt", "nested"]);

    fs::rename(dir.join("a.txt"), dir.join("nested/renamed.txt")).unwrap();
    assert_eq!(fs::read_to_string(dir.join("nested/renamed.txt")).unwrap(), "a");
    assert!(!dir.join("a.txt").exists());

    fs::remove_dir_all(&dir).unwrap();
}

#[test]
fn buffered_line_reads_and_appends() {
    let dir = scratch_dir("fs-lines");
    let path = dir.join("lines.txt");

    fs::write(&path, "first\nsecond\n").unwrap();
    let mut f = fs::OpenOptions::new().append(true).open(&path).unwrap();
    writeln!(f, "third").unwrap();
    drop(f);

    let lines: Vec<String> = BufReader::new(fs::File::open(&path).unwrap())
        .lines()
        .map(|l| l.unwrap())
        .collect();
    assert_eq!(lines, ["first", "second", "third"]);

    fs::remove_dir_all(&dir).unwrap();
}

#[test]
fn missing_file_reports_not_found() {
    let err = fs::File::open("/tmp/wali-rust-tests/definitely-missing").unwrap_err();
    assert_eq!(err.kind(), std::io::ErrorKind::NotFound);
}
