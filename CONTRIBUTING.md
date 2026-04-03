# Contributing

Thanks for your interest in SUB ORBIT.

## Reporting Issues

Open a [GitHub Issue](https://github.com/echoeslabmusic/sub-orbit-plugin/issues) with:

- Your OS and DAW (name + version)
- Plugin format (VST3, AU)
- Steps to reproduce
- Expected vs actual behavior

## Pull Requests

1. Fork the repo and create a feature branch from `main`
2. Run `clang-format` before committing (CI enforces this)
3. Add tests for new DSP or parameter behavior
4. Verify all tests pass: `ctest --test-dir build --output-on-failure`
5. Open a PR with a clear description of what changed and why

## Code Style

- C++20, JUCE conventions
- clang-format enforced (config in `.clang-format`)
- Allman braces, 4-space indent, 120 column limit
- See [DEVELOPMENT.md](DEVELOPMENT.md) for build and test instructions

## Audio Thread Safety

Code running on the audio thread must:

- Never allocate or free memory
- Never lock a mutex
- Use `std::atomic` or lock-free structures for cross-thread data
- Use `juce::SmoothedValue` for parameter changes to avoid zipper noise

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
