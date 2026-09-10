# cmdsh

cmdsh is a small POSIX shell written in C.

Unlike traditional shells such as `sh`, `bash`, or `zsh`, cmdsh does not aim to provide a full shell programming language. It is intended to remain a small command execution environment.

## Supported systems

cmdsh is primarily developed on Unix-like systems.

It should work on systems providing the usual POSIX process, file descriptor, and terminal interfaces.

## Dependencies

A C99-compatible compiler is required to build cmdsh.

[cbld](https://github.com/drwxor/cbld) is used as the build system.

## Building

Build cmdsh with:

```sh
cbld build
```

The resulting binary is written to the current working directory.

## Development

The project aims to remain small and stable. Changes are primarily expected to be fixes, portability improvements, and small additions that fit the existing design.

## License

See `LICENSE`.
