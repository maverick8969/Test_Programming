# ESP-IDF Project Structure

This project uses ESP-IDF framework, which uses `main/` directory instead of `src/`.

**DO NOT put source files here!**

For ESP-IDF projects:
- Source files go in: `main/`
- Test files are in: `test/test_XX/`
- The pre-build script copies test files to `main/` before compilation

This `src/` directory exists only to satisfy PlatformIO's requirement.
The actual source code is in `main/` directory.

See: https://docs.platformio.org/en/latest/frameworks/espidf.html
