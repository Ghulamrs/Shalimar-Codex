# rev19: IR-native borrowed math functions

Implemented IR-native emission for the scalar math borrow list, mapping `abs` to `fabs`, `min` to `fmin`, and `max` to `fmax`. The compiler was built with C++98 and the original 12-program backend-parity suite was rerun without modifying inputs.

**Measured exact output parity: 6/12** (up from 4/12). Six cases remain unsupported. This is an incremental checkpoint, not complete P1/P2 implementation.

| Program | Exact parity | IR diagnostic |
|---|---|---|
| factorial.shm | PASS |  |
| fibonacci.shm | PASS |  |
| gaussseidel.shm | FAIL | shalimarc: IR-native code generation error: solve: IR-native backend supports scalar int/real/char parameters (including scalar references), rank-one/rank-two int arrays, and rank-one char string parameters |
| gcd.shm | PASS |  |
| invert.shm | FAIL | shalimarc: IR-native code generation error: invert: IR-native backend supports scalar int/real/char parameters (including scalar references), rank-one/rank-two int arrays, and rank-one char string parameters |
| prime.shm | FAIL | shalimarc: IR-native code generation error: prime: IR-native backend does not yet support real-valued for counters |
| quadratic.shm | PASS |  |
| rotations.shm | FAIL | shalimarc: IR-native code generation error: IR-native backend does not support assigning an array literal to a scalar |
| rotmat.shm | FAIL | shalimarc: IR-native code generation error: ROX: IR-native backend supports scalar int/real/char parameters (including scalar references), rank-one/rank-two int arrays, and rank-one char string parameters |
| sqroot.shm | PASS |  |
| strsplit.shm | FAIL | shalimarc: IR-native code generation error: IR-native backend does not support IR operation: break |
| table.shm | PASS |  |

