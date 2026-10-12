# rev18 — backend parity audit

The original twelve specification examples were compiled and executed on Linux using both the default AST-to-C backend and the experimental IR-native backend in Release mode. Output comparisons are exact, not numerical-tolerance comparisons.

| Example | AST runtime | IR-native | Exact output parity |
|---|---|---|---|
| factorial.shm | PASS | PASS | PASS |
| fibonacci.shm | PASS | PASS | PASS |
| gaussseidel.shm | PASS | UNSUPPORTED/FAIL | NO |
| gcd.shm | PASS | PASS | PASS |
| invert.shm | PASS | UNSUPPORTED/FAIL | NO |
| prime.shm | PASS | UNSUPPORTED/FAIL | NO |
| quadratic.shm | PASS | UNSUPPORTED/FAIL | NO |
| rotations.shm | PASS | UNSUPPORTED/FAIL | NO |
| rotmat.shm | PASS | UNSUPPORTED/FAIL | NO |
| sqroot.shm | PASS | UNSUPPORTED/FAIL | NO |
| strsplit.shm | PASS | UNSUPPORTED/FAIL | NO |
| table.shm | PASS | PASS | PASS |

**Result:** 12/12 AST executions successful; 4/12 IR-native executions successful with exact output parity. Eight IR-native programs are not yet supported. This is an audit checkpoint, NOT completion of backend parity.

## IR-native diagnostic excerpts

- `gaussseidel.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`
- `invert.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`
- `prime.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`
- `quadratic.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`
- `rotations.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`
- `rotmat.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`
- `sqroot.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`
- `strsplit.shm`: `shalimarc: IR-native code generation error: IR-native backend currently supports only the borrowed len function`

The test inputs were not changed. Output equality does not prove numerical correctness. Windows and macOS remain unverified.
