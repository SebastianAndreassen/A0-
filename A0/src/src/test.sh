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

# Secret file
printf "hemmelighed" > test_files/hemmelig.input
chmod 000 test_files/hemmelig.input


# ISO-8859-1 (Latin-1) — use iconv to force encoding
printf "Héllo, wörld!\n" | iconv -t ISO-8859-1 > test_files/iso8859.input
printf "España\n" | iconv -t ISO-8859-1 > test_files/iso8859_spanish.input
printf 'Latin-1: \xE9 \xF1 \xA0\n' > test_files/iso8859_bytes.input
printf '\xE9\n' > test_files/iso8859_e9_only.input
hexdump -C test_files/iso8859_e9_only.input
LC_ALL=C file test_files/iso8859_e9_only.input
file -i test_files/iso8859_e9_only.input

echo "Running the tests.."
exitcode=0
for f in test_files/*.input test_files/does_not_exist.input; do
  echo ">>> Testing ${f}.."


  file "${f}" | sed -e 's/ASCII text.*/ASCII text/' \
                  -e 's/Unicode text, UTF-8 text.*/UTF-8 Unicode text/' \
                  -e 's/ISO-8859 text.*/ISO-8859 text/' \
                  -e 's/regular file, no read permission*/cannot determine (Permission denied)/' \
                  -e 's|^\([^:]*\): cannot open .*(No such file or directory).*|\1: cannot determine (No such file or directory)|' \
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
