# Contributing to FMAN

First off, thank you for considering contributing to FMAN! It's people like you that make open-source such a great community.

##  How to Contribute

### 1. Reporting Bugs
If you find a bug, please open an issue and include:
* Your operating system and compiler version.
* A clear and concise description of the bug.
* Steps to reproduce the behavior (e.g., the specific CLI command or DSL `.fman` script used).
* The Valgrind output or stack trace if it's a memory issue, segmentation fault, or crash.

### 2. Suggesting Enhancements
Enhancement suggestions are always welcome. Please open an issue detailing:
* The problem it solves or the missing feature.
* How it fits into the parallel runtime or DSL architecture.

### 3. Pull Requests
When you are ready to submit a PR:
1. Fork the repository and create your feature branch from `main`.
2. Write clean, readable ISO C99 code.
3. Keep your commits atomic and use descriptive commit messages.
4. Ensure your code does not introduce memory leaks (see the Testing section).

##  Development & Testing

FMAN is built on native C, and we take memory safety very seriously. Before opening a pull request, you **must** run the validation pipeline.

```bash
# Run the complete validation pipeline
./scripts/check.sh
