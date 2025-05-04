# Contributing

## Testing

After any change to the detection routines or match tables, it's always
a good idea to run `make test`. If some test fails, and you're confident
that the test is wrong and needs fixing, run `make fix-tests`.

You can also add a new test (which is basically a file containing
the raw CPUID data and the expected decoded items) by using
`tests/create_test.py`. The workflow there is as follows:

1. Run `cpuid_tool` with no arguments. It will tell you that it
   has written a pair of files, `raw.txt` and `report.txt`. Ensure
   that `report.txt` contains meaningful data.
2. Run `./tests/create_test.py raw.txt report.txt > my-cpu.test`
3. Use a proper descriptive name for the test, like the CPU brand name in [*kebab-case*](https://developer.mozilla.org/en-US/docs/Glossary/Kebab_case)
   (look into [`tests/amd`](tests/amd) and [`tests/intel`](tests/intel) to get an idea),
   and copy your test file to an appropriate place within the tests directory hierarchy.

AIDA64 CPUID dumps (mostly found on [InstLatx64](http://instlatx64.atw.hu/)) are also supported.
To create a new test from InstLatx64, you can use the [`create_test_from_instlatx64.sh`](utils/create_test_from_instlatx64.sh) script.

For non-developers, who still want to contribute tests for the project,
use [this page](https://github.com/anrieff/libcpuid/issues/new) to report
misdetections or new CPUs that libcpuid doesn't handle well yet.
