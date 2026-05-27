# WALI Specification

This directory holds the source-of-truth of the WALI specification, and 
scaffolding tooling for auto-generating bindings.
See [wali-linux](https://pypi.org/project/wali-linux/) on PyPI for package
usage and public API.

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
