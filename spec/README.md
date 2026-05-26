# WALI Spec Tooling

This directory holds the WALI specification as an installable Python package
(`wali`, in [`wali/`](wali/)) plus the tooling for auto-generating bindings. For package
usage and the public API, see [PYPI.md](PYPI.md) (the published description).

## Auto-Generated Bindings 

```sh
python3 autogen.py all          # everything: libc, wamr, wit, docs
python3 autogen.py libc wamr    # only selected generators
python3 autogen.py -h           # list generators and options
```

## Documentation Site

Docs are rendered with MkDocs.

```sh
pip install mkdocs-material mkdocs-macros-plugin pyyaml
python3 autogen.py docs         # generate the markdown
mkdocs serve                    # live preview at http://127.0.0.1:8000
```