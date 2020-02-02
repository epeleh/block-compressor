# Block Compressor
Command line application for compressing files.<br />
Sometimes it works well and performs a good compression ratio, but usually **gzip** works better..

## Usage examples
**Compress (verbose option):**
```bash
$ cat foo.txt
12345
12345
12345
12345
$ bczip -v foo.txt
bczip: 'foo.txt'  45.8% replaced with 'foo.txt.bc'
```

**Decompress (using stdin):**
```bash
$ bczip -d < foo.txt.bc
12345
12345
12345
12345
```

**Help:**
```bash
$ bczip --help
Usage: bczip [OPTION]... [FILE]...
Compress or uncompress FILEs (by default, compress FILES in-place).

  -c, --stdout      write on standard output, keep original files unchanged
  -d, --decompress  decompress
  -f, --force       force overwrite of output file
  -h, --help        give this help
  -k, --keep        keep (don't delete) input files
  -q, --quiet       suppress all warnings
  -v, --verbose     verbose mode
  -V, --version     display version number

With no FILE, read standard input.
```

## Installation
```bash
$ apt-get update && apt-get install -y git gcc make
$ git clone https://github.com/epeleh/block-compressor.git
$ cd block-compressor/
$ make install
```
