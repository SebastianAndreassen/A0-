#!/usr/bin/env bash
# Exit immediately if any command below fails.
set -e

make

echo "Generating a test_files directory.."
mkdir -p test_files
rm -f test_files/*

echo "Generating test files.."
# ASCII
printf "Hello, World!\n" > test_files/ascii.input
printf "Hello, World!" > test_files/ascii2.input

# Contains a NUL byte (binary-ish)
printf "Hello,\x00World!\n" > test_files/data.input

# Empty
printf "" > test_files/empty.input

# UTF-8 Unicode (Danish letters + en dash)
printf "Hej, Verden – ø æ å\n" > test_files/utf8.input
# UTF-8 Unicode (CJK)
printf "你好，世界\n" > test_files/utf8_chinese.input

# ISO-8859-1 (Latin-1) — use iconv to force encoding
printf "Héllo, wörld!\n" | iconv -t ISO-8859-1 > test_files/iso8859.input
printf "España\n" | iconv -t ISO-8859-1 > test_files/iso8859_spanish.input

echo "Running the tests.."
exitcode=0
for f in test_files/*.input; do
  echo ">>> Testing ${f}.."

  # Normalize common variations from system 'file' output to canonical labels
  file "${f}" | sed -e 's/ASCII text.*/ASCII text/' \
                    -e 's/UTF-8 Unicode text.*/UTF-8 Unicode text/' \
                    -e 's/Unicode text, UTF-8 text.*/UTF-8 Unicode text/' \
                    -e 's/ISO-885 text.*/ISO-885 text/' \
                    -e 's/writable, regular file, no read permission/cannot determine (Permission denied)/' \
                    > "${f}.expected"

  ./file "${f}" > "${f}.actual"

  if ! diff -u "${f}.expected" "${f}.actual"; then
    echo ">>> Failed 🙁"
    exitcode=1
  else
    echo ">>> Success 🙂"
  fi
done

exit $exitcode
