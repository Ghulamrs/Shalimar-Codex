#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR="${SHALIMAR_BUILD_DIR:-$ROOT/build-smoke}"
mkdir -p "$BUILD_DIR"
if [ -n "${SHALIMAR_COMPILER:-}" ]; then
    COMPILER="$SHALIMAR_COMPILER"
else
    cmake -S "$ROOT" -B "$BUILD_DIR"
    cmake --build "$BUILD_DIR"
    COMPILER="$BUILD_DIR/shalimarc"
fi

"$COMPILER" "$ROOT/examples/hello.shl" --check

# Expression precedence, right-associative power, unary-minus binding, numeric
# conversions, and int/real results must agree across both code-generation paths.
"$COMPILER" "$ROOT/tests/expression_precedence.shl" --emit-c "$BUILD_DIR/expression_precedence_ast.c" --release
"$COMPILER" "$ROOT/tests/expression_precedence.shl" --emit-c "$BUILD_DIR/expression_precedence_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/expression_precedence_ast.c" -lm -o "$BUILD_DIR/expression_precedence_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/expression_precedence_ir.c" -lm -o "$BUILD_DIR/expression_precedence_ir"
AST_OUTPUT=$("$BUILD_DIR/expression_precedence_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/expression_precedence_ir" | sed 's/ *$//')
EXPECTED=$(printf '4.0000000\n0.2500000\n512.0000000\n14\n20\n5\n1\n2\n1.5000000\n2 -2\nA\n8.0000000\n8.0000000 3.0000000')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST expression-precedence output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected IR-native expression-precedence output: [$IR_OUTPUT]" >&2
    exit 1
}
# Integer arithmetic and numeric conversions use explicit runtime guards in
# both code-generation backends. Valid cases retain parity; invalid cases fail
# with a deterministic Shalimar diagnostic instead of C undefined behavior.
"$COMPILER" "$ROOT/tests/ir_numeric_runtime_checks.shl" --emit-c "$BUILD_DIR/numeric_checks_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_numeric_runtime_checks.shl" --emit-c "$BUILD_DIR/numeric_checks_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/numeric_checks_ast.c" -lm -o "$BUILD_DIR/numeric_checks_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/numeric_checks_ir.c" -lm -o "$BUILD_DIR/numeric_checks_ir"
AST_OUTPUT=$("$BUILD_DIR/numeric_checks_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/numeric_checks_ir" | sed 's/ *$//')
EXPECTED=$(printf '42 42 4\n2 -2 A')
[ "$AST_OUTPUT" = "$EXPECTED" ] || { echo "unexpected AST numeric-check output: [$AST_OUTPUT]" >&2; exit 1; }
[ "$IR_OUTPUT" = "$EXPECTED" ] || { echo "unexpected IR numeric-check output: [$IR_OUTPUT]" >&2; exit 1; }
check_numeric_failure() {
    source=$1
    message=$2
    backend=$3
    suffix=$backend
    extra=
    if [ "$backend" = ir ]; then extra="--ir-native"; fi
    "$COMPILER" "$ROOT/tests/$source" --emit-c "$BUILD_DIR/${source%.shl}_${suffix}.c" --release $extra
    cc -std=c99 "$BUILD_DIR/${source%.shl}_${suffix}.c" -lm -o "$BUILD_DIR/${source%.shl}_${suffix}"
    (ulimit -c 0; if "$BUILD_DIR/${source%.shl}_${suffix}" >"$BUILD_DIR/${source%.shl}_${suffix}.out" 2>"$BUILD_DIR/${source%.shl}_${suffix}.err"; then
        echo "numeric safety test unexpectedly succeeded: $source ($backend)" >&2
        exit 1
    fi)
    grep -F "$message" "$BUILD_DIR/${source%.shl}_${suffix}.err" >/dev/null || {
        echo "numeric safety diagnostic missing for $source ($backend)" >&2
        exit 1
    }
}
for backend in ast ir; do
    check_numeric_failure ir_numeric_overflow_add.shl "Shalimar integer overflow" "$backend"
    check_numeric_failure ir_numeric_overflow_multiply.shl "Shalimar integer overflow" "$backend"
    check_numeric_failure ir_numeric_overflow_negate.shl "Shalimar integer overflow" "$backend"
    check_numeric_failure ir_numeric_divide_zero.shl "Shalimar division by zero" "$backend"
    check_numeric_failure ir_numeric_divide_overflow.shl "Shalimar integer overflow" "$backend"
    check_numeric_failure ir_numeric_remainder_zero.shl "Shalimar remainder by zero" "$backend"
    check_numeric_failure ir_numeric_int_conversion_range.shl "Shalimar int conversion out of range" "$backend"
    check_numeric_failure ir_numeric_int_conversion_nonfinite.shl "Shalimar int conversion out of range" "$backend"
    check_numeric_failure ir_numeric_char_conversion_range.shl "Shalimar char conversion out of range" "$backend"
    check_numeric_failure ir_numeric_char_conversion_negative.shl "Shalimar char conversion out of range" "$backend"
done

"$COMPILER" "$ROOT/tests/functions.shl" --emit-c "$BUILD_DIR/functions.c"
cc -std=c99 "$BUILD_DIR/functions.c" -lm -o "$BUILD_DIR/functions"
OUTPUT=$("$BUILD_DIR/functions" | sed 's/ *$//')
[ "$OUTPUT" = "49" ] || {
    echo "unexpected scalar-function output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/arrays.shl" --emit-c "$BUILD_DIR/arrays.c"
cc -std=c99 "$BUILD_DIR/arrays.c" -lm -o "$BUILD_DIR/arrays"
OUTPUT=$("$BUILD_DIR/arrays" | sed 's/ *$//')
[ "$OUTPUT" = "40" ] || {
    echo "unexpected array output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/runtime_sized_arrays.shl" --emit-c "$BUILD_DIR/runtime_sized_arrays.c"
cc -std=c99 "$BUILD_DIR/runtime_sized_arrays.c" -lm -o "$BUILD_DIR/runtime_sized_arrays"
OUTPUT=$("$BUILD_DIR/runtime_sized_arrays" | sed 's/ *$//')
EXPECTED=$(printf '2 3 1 2 0\n3 0 4\n5 7 8 0 0\n20')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected runtime-sized-array output: $OUTPUT" >&2
    exit 1
}

check_runtime_array_failure() {
    name=$1
    source=$2
    expected_message=$3
    "$COMPILER" "$ROOT/tests/$source" --emit-c "$BUILD_DIR/$name.c"
    cc -std=c99 "$BUILD_DIR/$name.c" -lm -o "$BUILD_DIR/$name"
    (ulimit -c 0; if "$BUILD_DIR/$name" >"$BUILD_DIR/$name.out" 2>"$BUILD_DIR/$name.err"; then
        echo "runtime-sized-array failure test unexpectedly succeeded: $name" >&2
        exit 1
    fi)
    grep -q "$expected_message" "$BUILD_DIR/$name.err"
}

check_runtime_array_failure runtime_array_invalid_dimension runtime_array_invalid_dimension.shl "Shalimar array dimension must be positive"
check_runtime_array_failure runtime_array_initializer_too_large runtime_array_initializer_too_large.shl "Shalimar array initializer exceeds runtime dimension"

"$COMPILER" "$ROOT/tests/array_parameter.shl" --emit-c "$BUILD_DIR/array_parameter.c"
cc -std=c99 "$BUILD_DIR/array_parameter.c" -lm -o "$BUILD_DIR/array_parameter"
OUTPUT=$("$BUILD_DIR/array_parameter" | sed 's/ *$//')
[ "$OUTPUT" = "42" ] || {
    echo "unexpected array-parameter output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/array_parameter_dimensions.shl" --emit-c "$BUILD_DIR/array_parameter_dimensions.c"
cc -std=c99 "$BUILD_DIR/array_parameter_dimensions.c" -lm -o "$BUILD_DIR/array_parameter_dimensions"
OUTPUT=$("$BUILD_DIR/array_parameter_dimensions" | sed 's/ *$//')
EXPECTED=$(printf '14\n25')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected array-parameter dimension output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/multidimensional_arrays.shl" --emit-c "$BUILD_DIR/multidimensional_arrays.c"
cc -std=c99 "$BUILD_DIR/multidimensional_arrays.c" -lm -o "$BUILD_DIR/multidimensional_arrays"
OUTPUT=$("$BUILD_DIR/multidimensional_arrays" | sed 's/ *$//')
[ "$OUTPUT" = "15" ] || {
    echo "unexpected multidimensional-array output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/array_bounds_valid.shl" --emit-c "$BUILD_DIR/array_bounds_valid.c"
cc -std=c99 "$BUILD_DIR/array_bounds_valid.c" -lm -o "$BUILD_DIR/array_bounds_valid"
OUTPUT=$("$BUILD_DIR/array_bounds_valid" | sed 's/ *$//')
EXPECTED=$(printf '11 33\n6\n22')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected valid array-bounds output: $OUTPUT" >&2
    exit 1
}

check_array_bounds_failure() {
    name=$1
    source=$2
    "$COMPILER" "$ROOT/tests/$source" --emit-c "$BUILD_DIR/$name.c"
    cc -std=c99 "$BUILD_DIR/$name.c" -lm -o "$BUILD_DIR/$name"
    (ulimit -c 0; if "$BUILD_DIR/$name" >"$BUILD_DIR/$name.out" 2>"$BUILD_DIR/$name.err"; then
        echo "out-of-bounds test unexpectedly succeeded: $name" >&2
        exit 1
    fi)
    grep -q "Shalimar array index out of bounds" "$BUILD_DIR/$name.err"
}

check_array_bounds_failure array_bounds_negative array_bounds_negative.shl
check_array_bounds_failure array_bounds_upper array_bounds_upper.shl
check_array_bounds_failure array_bounds_multidimensional array_bounds_multidimensional.shl
check_array_bounds_failure array_bounds_parameter array_bounds_parameter.shl

"$COMPILER" "$ROOT/tests/array_initializer_rules.shl" --emit-c "$BUILD_DIR/array_initializer_rules.c"
cc -std=c99 "$BUILD_DIR/array_initializer_rules.c" -lm -o "$BUILD_DIR/array_initializer_rules"
OUTPUT=$("$BUILD_DIR/array_initializer_rules" | sed 's/ *$//')
[ "$OUTPUT" = "1 0 3 4 0 0" ] || {
    echo "unexpected array-initializer output: $OUTPUT" >&2
    exit 1
}

if "$COMPILER" "$ROOT/tests/array_initializer_too_many.shl" --check >"$BUILD_DIR/array_initializer_too_many.log" 2>&1; then
    echo "oversized array initializer unexpectedly succeeded" >&2
    exit 1
fi
grep -q "more entries than dimension" "$BUILD_DIR/array_initializer_too_many.log"

if "$COMPILER" "$ROOT/tests/array_initializer_bad_shape.shl" --check >"$BUILD_DIR/array_initializer_bad_shape.log" 2>&1; then
    echo "multidimensional initializer with scalar row unexpectedly succeeded" >&2
    exit 1
fi
grep -q "each row of a multidimensional array initializer must be an array literal" "$BUILD_DIR/array_initializer_bad_shape.log"

"$COMPILER" "$ROOT/tests/multi_output.shl" --emit-c "$BUILD_DIR/multi_output.c"
cc -std=c99 "$BUILD_DIR/multi_output.c" -lm -o "$BUILD_DIR/multi_output"
OUTPUT=$("$BUILD_DIR/multi_output" | sed 's/ *$//')
[ "$OUTPUT" = "3 2" ] || {
    echo "unexpected multi-output result: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/array_attributes.shl" --emit-c "$BUILD_DIR/array_attributes.c"
cc -std=c99 "$BUILD_DIR/array_attributes.c" -lm -o "$BUILD_DIR/array_attributes"
OUTPUT=$("$BUILD_DIR/array_attributes" | sed 's/ *$//')
EXPECTED=$(printf '4 -1 4 -1\n2 3 3 -1\n3 -1')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected array-attribute output: $OUTPUT" >&2
    exit 1
}

cat > "$BUILD_DIR/scalar_attribute_error.shl" <<'EOF'
fun main() {
    int scalar : 3
    ? scalar.row
}
EOF
if "$COMPILER" "$BUILD_DIR/scalar_attribute_error.shl" --check >/dev/null 2>&1; then
    echo "scalar-attribute test unexpectedly succeeded" >&2
    exit 1
fi

if "$COMPILER" "$ROOT/tests/semantic_error.shl" --check >/dev/null 2>&1; then
    echo "semantic-error test unexpectedly succeeded" >&2
    exit 1
fi

"$COMPILER" "$ROOT/tests/scalar_reference.shl" --emit-c "$BUILD_DIR/scalar_reference.c"
cc -std=c99 "$BUILD_DIR/scalar_reference.c" -lm -o "$BUILD_DIR/scalar_reference"
OUTPUT=$("$BUILD_DIR/scalar_reference" | sed 's/ *$//')
[ "$OUTPUT" = "6" ] || {
    echo "unexpected scalar-reference output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/scalar_reference_element.shl" --emit-c "$BUILD_DIR/scalar_reference_element.c"
cc -std=c99 "$BUILD_DIR/scalar_reference_element.c" -lm -o "$BUILD_DIR/scalar_reference_element"
OUTPUT=$("$BUILD_DIR/scalar_reference_element" | sed 's/ *$//')
[ "$OUTPUT" = "22" ] || {
    echo "unexpected scalar-reference element output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/scalar_reference_copyback.shl" --emit-c "$BUILD_DIR/scalar_reference_copyback.c"
cc -std=c99 "$BUILD_DIR/scalar_reference_copyback.c" -lm -o "$BUILD_DIR/scalar_reference_copyback"
OUTPUT=$("$BUILD_DIR/scalar_reference_copyback" | sed 's/ *$//')
[ "$OUTPUT" = "7" ] || {
    echo "unexpected scalar-reference copy-back output: $OUTPUT" >&2
    exit 1
}

if "$COMPILER" "$ROOT/tests/scalar_reference_reject_literal.shl" --check >/dev/null 2>&1; then
    echo "reference-literal test unexpectedly succeeded" >&2
    exit 1
fi


"$COMPILER" "$ROOT/tests/globals.shl" --emit-c "$BUILD_DIR/globals.c"
cc -std=c99 "$BUILD_DIR/globals.c" -lm -o "$BUILD_DIR/globals"
OUTPUT=$("$BUILD_DIR/globals" | sed 's/ *$//')
EXPECTED=$(printf '10 12\n13')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected global-variable output: $OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/global_arrays.shl" --emit-c "$BUILD_DIR/global_arrays.c"
cc -std=c99 "$BUILD_DIR/global_arrays.c" -lm -o "$BUILD_DIR/global_arrays"
OUTPUT=$("$BUILD_DIR/global_arrays" | sed 's/ *$//')
EXPECTED=$(printf '1 0 3 0\n1 3 4 0\n4 -1 4 4\n2 3 3\nabcd\n11')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected global-array output: $OUTPUT" >&2
    exit 1
}

cat > "$BUILD_DIR/global_array_bad_dimension.shl" <<'EOF'
int invalid[2.5] : {1}
fun main() {
    ? 1
}
EOF
if "$COMPILER" "$BUILD_DIR/global_array_bad_dimension.shl" --check >"$BUILD_DIR/global_array_bad_dimension.log" 2>&1; then
    echo "non-integer global-array dimension unexpectedly succeeded" >&2
    exit 1
fi
grep -q "global array dimensions must be int expressions" "$BUILD_DIR/global_array_bad_dimension.log"

if "$COMPILER" "$ROOT/tests/global_later_reference.shl" --check >"$BUILD_DIR/global_later_reference.log" 2>&1; then
    echo "later-global reference unexpectedly succeeded" >&2
    exit 1
fi
grep -q "global declared later" "$BUILD_DIR/global_later_reference.log"

if "$COMPILER" "$ROOT/tests/global_initializer_later_reference.shl" --check >"$BUILD_DIR/global_initializer_later_reference.log" 2>&1; then
    echo "global initializer reference to later global unexpectedly succeeded" >&2
    exit 1
fi
grep -q "global declared later" "$BUILD_DIR/global_initializer_later_reference.log"

"$COMPILER" "$ROOT/tests/library_functions.shl" --emit-c "$BUILD_DIR/library_functions.c"
cc -std=c99 "$BUILD_DIR/library_functions.c" -lm -o "$BUILD_DIR/library_functions"
OUTPUT=$("$BUILD_DIR/library_functions" | sed 's/ *$//')
[ "$OUTPUT" = "9.0000000 5 2.5000000 9 3.0000000 8.0000000 3" ] || {
    echo "unexpected borrowed-library output: $OUTPUT" >&2
    exit 1
}

if "$COMPILER" "$ROOT/tests/library_missing_borrow.shl" --check >"$BUILD_DIR/library_missing_borrow.log" 2>&1; then
    echo "library call without uses unexpectedly succeeded" >&2
    exit 1
fi
grep -q "must be borrowed with 'uses sqrt'" "$BUILD_DIR/library_missing_borrow.log"

if "$COMPILER" "$ROOT/tests/library_bad_arity.shl" --check >"$BUILD_DIR/library_bad_arity.log" 2>&1; then
    echo "wrong library arity unexpectedly succeeded" >&2
    exit 1
fi
grep -q "takes 1 argument(s), got 2" "$BUILD_DIR/library_bad_arity.log"

if "$COMPILER" "$ROOT/tests/library_bad_name.shl" --check >"$BUILD_DIR/library_bad_name.log" 2>&1; then
    echo "non-borrowable library name unexpectedly succeeded" >&2
    exit 1
fi
grep -q "cannot borrow 'printf'" "$BUILD_DIR/library_bad_name.log"

if "$COMPILER" "$ROOT/tests/library_name_collision.shl" --check >"$BUILD_DIR/library_name_collision.log" 2>&1; then
    echo "borrowed library name variable collision unexpectedly succeeded" >&2
    exit 1
fi
grep -q "is borrowed" "$BUILD_DIR/library_name_collision.log"

"$COMPILER" "$ROOT/tests/strings_precision.shl" --emit-c "$BUILD_DIR/strings_precision.c"
cc -std=c99 "$BUILD_DIR/strings_precision.c" -lm -o "$BUILD_DIR/strings_precision"
OUTPUT=$("$BUILD_DIR/strings_precision" | sed 's/ *$//')
EXPECTED=$(printf 'text Shalimar A\n0.333333333333\n0.333\n3\n0.3333333\n0.333333333333333314829616\n0.3333333')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected string/precision output:" >&2
    printf '%s\n' "$OUTPUT" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/char_array_truncation.shl" --emit-c "$BUILD_DIR/char_array_truncation.c"
cc -std=c99 "$BUILD_DIR/char_array_truncation.c" -lm -o "$BUILD_DIR/char_array_truncation"
OUTPUT=$("$BUILD_DIR/char_array_truncation" | sed 's/ *$//')
EXPECTED=$(printf 'abc\nabc')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected char-array truncation output: $OUTPUT" >&2
    exit 1
}

if "$COMPILER" "$ROOT/tests/prec_bad_arity.shl" --check >"$BUILD_DIR/prec_bad_arity.log" 2>&1; then
    echo "prec() with wrong arity unexpectedly succeeded" >&2
    exit 1
fi
grep -q "prec() requires exactly one integer argument" "$BUILD_DIR/prec_bad_arity.log"

if "$COMPILER" "$ROOT/tests/prec_bad_type.shl" --check >"$BUILD_DIR/prec_bad_type.log" 2>&1; then
    echo "prec() with real argument unexpectedly succeeded" >&2
    exit 1
fi
grep -q "prec() requires an integer argument" "$BUILD_DIR/prec_bad_type.log"

"$COMPILER" "$ROOT/examples/hello.shl" --object "$BUILD_DIR/hello.o"
"$COMPILER" "$ROOT/tests/source_mapping.shl" --emit-c "$BUILD_DIR/source_mapping.c"
grep -F "#line 2 \"$ROOT/tests/source_mapping.shl\"" "$BUILD_DIR/source_mapping.c" >/dev/null
cc -std=c99 "$BUILD_DIR/source_mapping.c" -lm -o "$BUILD_DIR/source_mapping"
OUTPUT=$("$BUILD_DIR/source_mapping" | sed 's/ *$//')
[ "$OUTPUT" = "42" ] || {
    echo "unexpected source-mapping test output: $OUTPUT" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/source_mapping.shl" --object "$BUILD_DIR/source_mapping.o"
if command -v readelf >/dev/null 2>&1; then
    readelf --debug-dump=decodedline "$BUILD_DIR/source_mapping.o" > "$BUILD_DIR/source_mapping.lines"
    grep -F "source_mapping.shl" "$BUILD_DIR/source_mapping.lines" >/dev/null
fi
"$COMPILER" "$ROOT/tests/array_literal_assignment.shl" --emit-c "$BUILD_DIR/array_literal_assignment.c"
cc -std=c99 "$BUILD_DIR/array_literal_assignment.c" -lm -o "$BUILD_DIR/array_literal_assignment"
OUTPUT=$("$BUILD_DIR/array_literal_assignment" | sed 's/ *$//')
EXPECTED=$(printf '9 8 0 0\n7 0 0 8 9 0')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected array-literal assignment output: $OUTPUT" >&2
    exit 1
}

if "$COMPILER" "$ROOT/tests/array_literal_assignment_too_many.shl" --check >"$BUILD_DIR/array_literal_assignment_too_many.log" 2>&1; then
    echo "oversized array-literal assignment unexpectedly succeeded" >&2
    exit 1
fi
grep -q "more entries than dimension" "$BUILD_DIR/array_literal_assignment_too_many.log"

"$COMPILER" "$ROOT/tests/global_array_reassignment.shl" --emit-c "$BUILD_DIR/global_array_reassignment.c"
cc -std=c99 "$BUILD_DIR/global_array_reassignment.c" -lm -o "$BUILD_DIR/global_array_reassignment"
OUTPUT=$("$BUILD_DIR/global_array_reassignment" | sed 's/ *$//')
EXPECTED=$(printf '9 8 0 0\n7 0 0 8 9 0')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected global-array reassignment output: $OUTPUT" >&2
    exit 1
}

if "$COMPILER" "$ROOT/tests/global_array_reassignment_too_many.shl" --check >"$BUILD_DIR/global_array_reassignment_too_many.log" 2>&1; then
    echo "oversized global-array reassignment unexpectedly succeeded" >&2
    exit 1
fi
grep -q "more entries than dimension" "$BUILD_DIR/global_array_reassignment_too_many.log"

"$COMPILER" "$ROOT/tests/array_parameter_reassignment.shl" --emit-c "$BUILD_DIR/array_parameter_reassignment.c"
cc -std=c99 "$BUILD_DIR/array_parameter_reassignment.c" -lm -o "$BUILD_DIR/array_parameter_reassignment"
OUTPUT=$("$BUILD_DIR/array_parameter_reassignment" | sed 's/ *$//')
EXPECTED=$(printf '9 8 0 0\n7 0 0 8 9 0')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected array-parameter reassignment output: $OUTPUT" >&2
    exit 1
}
(
    ulimit -c 0
    if "$COMPILER" "$ROOT/tests/array_parameter_reassignment_too_many.shl" --emit-c "$BUILD_DIR/array_parameter_reassignment_too_many.c" >/dev/null 2>&1; then
        cc -std=c99 "$BUILD_DIR/array_parameter_reassignment_too_many.c" -lm -o "$BUILD_DIR/array_parameter_reassignment_too_many"
        if "$BUILD_DIR/array_parameter_reassignment_too_many" >/dev/null 2>&1; then
            echo "oversized runtime array-parameter assignment unexpectedly succeeded" >&2
            exit 1
        fi
    else
        echo "valid runtime-sized array-parameter assignment unexpectedly failed compilation" >&2
        exit 1
    fi
)

# The dedicated Shalimar IR exposes explicit operations and control-flow blocks.
"$COMPILER" "$ROOT/tests/ir_control_flow.shl" --dump-ir >"$BUILD_DIR/ir_control_flow.txt"
grep -F "shalimar-ir v1" "$BUILD_DIR/ir_control_flow.txt" >/dev/null
grep -F "binary +" "$BUILD_DIR/ir_control_flow.txt" >/dev/null
grep -F "branch" "$BUILD_DIR/ir_control_flow.txt" >/dev/null
grep -F "successors:" "$BUILD_DIR/ir_control_flow.txt" >/dev/null

# Release IR optimization propagates constants, folds branches, and removes dead blocks.
"$COMPILER" "$ROOT/tests/ir_optimization.shl" --dump-ir --release >"$BUILD_DIR/ir_optimization.txt"
if grep -F "binary +" "$BUILD_DIR/ir_optimization.txt" >/dev/null ||
   grep -F "binary >" "$BUILD_DIR/ir_optimization.txt" >/dev/null; then
    echo "Release IR optimizer left foldable arithmetic in the IR" >&2
    exit 1
fi
if grep -F "999" "$BUILD_DIR/ir_optimization.txt" >/dev/null ||
   grep -F "bb2:" "$BUILD_DIR/ir_optimization.txt" >/dev/null; then
    echo "Release IR optimizer retained an unreachable branch" >&2
    exit 1
fi
"$COMPILER" "$ROOT/tests/ir_optimization.shl" --dump-ir --debug >"$BUILD_DIR/ir_optimization_debug.txt"
grep -F "binary +" "$BUILD_DIR/ir_optimization_debug.txt" >/dev/null
grep -F "binary >" "$BUILD_DIR/ir_optimization_debug.txt" >/dev/null
grep -F "999" "$BUILD_DIR/ir_optimization_debug.txt" >/dev/null

# Cross-block data-flow carries safe constants into successor blocks and folds their expressions.
"$COMPILER" "$ROOT/tests/ir_cross_block.shl" --dump-ir --release >"$BUILD_DIR/ir_cross_block.txt"
if grep -F "binary +" "$BUILD_DIR/ir_cross_block.txt" >/dev/null ||
   grep -F "999" "$BUILD_DIR/ir_cross_block.txt" >/dev/null; then
    echo "cross-block IR data-flow failed to fold constants or remove the unreachable branch" >&2
    exit 1
fi
grep -F "print 7" "$BUILD_DIR/ir_cross_block.txt" >/dev/null
grep -F "print.newline" "$BUILD_DIR/ir_cross_block.txt" >/dev/null
grep -F "return 6" "$BUILD_DIR/ir_cross_block.txt" >/dev/null
"$COMPILER" "$ROOT/tests/ir_cross_block.shl" --dump-ir --debug >"$BUILD_DIR/ir_cross_block_debug.txt"
grep -F "binary +" "$BUILD_DIR/ir_cross_block_debug.txt" >/dev/null
grep -F "999" "$BUILD_DIR/ir_cross_block_debug.txt" >/dev/null

# Release LICM hoists safe loop-invariant comparisons; Debug IR is left untouched.
"$COMPILER" "$ROOT/tests/ir_loop_invariant.shl" --dump-ir --release >"$BUILD_DIR/ir_loop_invariant.txt"
awk '
    /^  bb[0-9]+:/ { block = $0 }
    /binary </ && block == "  bb0:" { found = 1 }
    END { exit(found ? 0 : 1) }
' "$BUILD_DIR/ir_loop_invariant.txt" || {
    echo "Release IR optimizer did not hoist the loop-invariant comparison to the preheader" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_loop_invariant.shl" --dump-ir --debug >"$BUILD_DIR/ir_loop_invariant_debug.txt"
awk '
    /^  bb[0-9]+:/ { block = $0 }
    /binary </ && block == "  bb0:" { found = 1 }
    END { exit(found ? 1 : 0) }
' "$BUILD_DIR/ir_loop_invariant_debug.txt" || {
    echo "Debug IR unexpectedly hoisted a loop-invariant comparison" >&2
    exit 1
}

# Release IR recognizes only canonical, constant-stride induction variables.
"$COMPILER" "$ROOT/tests/ir_induction_variable.shl" --dump-ir --release >"$BUILD_DIR/ir_induction_variable.txt"
grep -F "loop-induction i start=0 step=2 limit=10" "$BUILD_DIR/ir_induction_variable.txt" >/dev/null
"$COMPILER" "$ROOT/tests/ir_induction_variable.shl" --dump-ir --debug >"$BUILD_DIR/ir_induction_variable_debug.txt"
if grep -F "loop-induction" "$BUILD_DIR/ir_induction_variable_debug.txt" >/dev/null; then
    echo "Debug IR unexpectedly contains Release induction analysis" >&2
    exit 1
fi
"$COMPILER" "$ROOT/tests/ir_induction_variable_unsafe.shl" --dump-ir --release >"$BUILD_DIR/ir_induction_variable_unsafe.txt"
if grep -F "loop-induction" "$BUILD_DIR/ir_induction_variable_unsafe.txt" >/dev/null; then
    echo "induction analysis accepted a loop that modifies its induction variable in the body" >&2
    exit 1
fi
"$COMPILER" "$ROOT/tests/ir_induction_variable.shl" --emit-c "$BUILD_DIR/ir_induction_variable.c" --release
cc -std=c99 "$BUILD_DIR/ir_induction_variable.c" -lm -o "$BUILD_DIR/ir_induction_variable"
OUTPUT=$("$BUILD_DIR/ir_induction_variable" | sed 's/ *$//')
[ "$OUTPUT" = "30" ] || {
    echo "induction-analysis fixture behavior changed: $OUTPUT" >&2
    exit 1
}

# Release IR applies strength reduction only when a literal loop range proves
# the original product and every recurrence update (including the final update)
# remain in the Shalimar int range. Native AST-to-C code generation is unchanged.
"$COMPILER" "$ROOT/tests/ir_strength_reduction.shl" --dump-ir --release >"$BUILD_DIR/ir_strength_reduction.txt"
grep -F "strength-reduction applied expression=" "$BUILD_DIR/ir_strength_reduction.txt" >/dev/null
grep -F "induction=i factor=3 recurrence-step=6" "$BUILD_DIR/ir_strength_reduction.txt" >/dev/null
grep -F "recurrence=%sr0" "$BUILD_DIR/ir_strength_reduction.txt" >/dev/null
grep -F "%sr0 = binary + %sr0, 6" "$BUILD_DIR/ir_strength_reduction.txt" >/dev/null
if grep -F "%t1 = binary * i, 3" "$BUILD_DIR/ir_strength_reduction.txt" >/dev/null; then
    echo "Release IR failed to replace the proven multiplication" >&2
    exit 1
fi
"$COMPILER" "$ROOT/tests/ir_strength_reduction.shl" --dump-ir --debug >"$BUILD_DIR/ir_strength_reduction_debug.txt"
if grep -F "strength-reduction applied" "$BUILD_DIR/ir_strength_reduction_debug.txt" >/dev/null; then
    echo "Debug IR unexpectedly applied Release strength reduction" >&2
    exit 1
fi
"$COMPILER" "$ROOT/tests/ir_strength_reduction_unsafe.shl" --dump-ir --release >"$BUILD_DIR/ir_strength_reduction_unsafe.txt"
if grep -F "strength-reduction applied" "$BUILD_DIR/ir_strength_reduction_unsafe.txt" >/dev/null; then
    echo "strength reduction accepted a loop with an overflowing next induction/product value or a zero-trip loop" >&2
    exit 1
fi
"$COMPILER" "$ROOT/tests/ir_strength_reduction_nondivisible.shl" --dump-ir --release >"$BUILD_DIR/ir_strength_reduction_nondivisible.txt"
grep -F "strength-reduction applied" "$BUILD_DIR/ir_strength_reduction_nondivisible.txt" >/dev/null
"$COMPILER" "$ROOT/tests/ir_strength_reduction_nondivisible.shl" --emit-c "$BUILD_DIR/ir_strength_reduction_nondivisible.c" --release
cc -std=c99 "$BUILD_DIR/ir_strength_reduction_nondivisible.c" -lm -o "$BUILD_DIR/ir_strength_reduction_nondivisible"
OUTPUT=$("$BUILD_DIR/ir_strength_reduction_nondivisible" | sed 's/ *$//')
[ "$OUTPUT" = "60" ] || {
    echo "nondivisible-limit strength-reduction fixture behavior changed: $OUTPUT" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_strength_reduction_negative_stride.shl" --dump-ir --release >"$BUILD_DIR/ir_strength_reduction_negative_stride.txt"
grep -F "strength-reduction applied" "$BUILD_DIR/ir_strength_reduction_negative_stride.txt" >/dev/null
"$COMPILER" "$ROOT/tests/ir_strength_reduction_negative_stride.shl" --emit-c "$BUILD_DIR/ir_strength_reduction_negative_stride.c" --release
cc -std=c99 "$BUILD_DIR/ir_strength_reduction_negative_stride.c" -lm -o "$BUILD_DIR/ir_strength_reduction_negative_stride"
OUTPUT=$("$BUILD_DIR/ir_strength_reduction_negative_stride" | sed 's/ *$//')
[ "$OUTPUT" = "90" ] || {
    echo "negative-stride strength-reduction fixture behavior changed: $OUTPUT" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_strength_reduction.shl" --emit-c "$BUILD_DIR/ir_strength_reduction.c" --release
cc -std=c99 "$BUILD_DIR/ir_strength_reduction.c" -lm -o "$BUILD_DIR/ir_strength_reduction"
OUTPUT=$("$BUILD_DIR/ir_strength_reduction" | sed 's/ *$//')
[ "$OUTPUT" = "90" ] || {
    echo "strength-reduction fixture behavior changed: $OUTPUT" >&2
    exit 1
}


# The opt-in IR-native backend compiles optimized scalar IR to native C. Compare
# observable output with the established AST-to-C backend.
"$COMPILER" "$ROOT/tests/ir_strength_reduction.shl" --emit-c "$BUILD_DIR/ir_native_strength.c" --release --ir-native
grep -F "__ir_sr0" "$BUILD_DIR/ir_native_strength.c" >/dev/null
if grep -F "i * 3" "$BUILD_DIR/ir_native_strength.c" >/dev/null; then
    echo "IR-native backend did not consume the strength-reduced IR" >&2
    exit 1
fi
cc -std=c99 "$BUILD_DIR/ir_native_strength.c" -lm -o "$BUILD_DIR/ir_native_strength"
OUTPUT=$("$BUILD_DIR/ir_native_strength" | sed 's/ *$//')
[ "$OUTPUT" = "90" ] || {
    echo "IR-native strength-reduction result mismatch: $OUTPUT" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_call.shl" --emit-c "$BUILD_DIR/ir_native_call.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_call.c" -lm -o "$BUILD_DIR/ir_native_call"
OUTPUT=$("$BUILD_DIR/ir_native_call" | sed 's/ *$//')
[ "$OUTPUT" = "21 22" ] || {
    echo "IR-native function-call result mismatch: $OUTPUT" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_call.shl" --object "$BUILD_DIR/ir_native_call.o" --release --ir-native >/dev/null
"$COMPILER" "$ROOT/tests/ir_control_flow.shl" --emit-c "$BUILD_DIR/ir_native_control_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_control_flow.shl" --emit-c "$BUILD_DIR/ir_native_control_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_control_ast.c" -lm -o "$BUILD_DIR/ir_native_control_ast"
cc -std=c99 "$BUILD_DIR/ir_native_control_ir.c" -lm -o "$BUILD_DIR/ir_native_control_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_control_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_control_ir" | sed 's/ *$//')
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native branch/loop output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
# The IR-native backend now supports fixed-size one-dimensional local int arrays.
"$COMPILER" "$ROOT/tests/ir_native_arrays.shl" --emit-c "$BUILD_DIR/ir_native_arrays_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_arrays.shl" --emit-c "$BUILD_DIR/ir_native_arrays_ir.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic "$BUILD_DIR/ir_native_arrays_ast.c" -lm -o "$BUILD_DIR/ir_native_arrays_ast"
cc -std=c99 -Wall -Wextra -pedantic "$BUILD_DIR/ir_native_arrays_ir.c" -lm -o "$BUILD_DIR/ir_native_arrays_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_arrays_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_arrays_ir" | sed 's/ *$//')
EXPECTED=$(printf '10 10 0 0\n3 0 0 0')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST fixed-array output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native fixed-array output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_array_bounds.shl" --emit-c "$BUILD_DIR/ir_native_array_bounds.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_array_bounds.c" -lm -o "$BUILD_DIR/ir_native_array_bounds"
(ulimit -c 0; if "$BUILD_DIR/ir_native_array_bounds" >"$BUILD_DIR/ir_native_array_bounds.out" 2>"$BUILD_DIR/ir_native_array_bounds.err"; then
    echo "IR-native array bounds test unexpectedly succeeded" >&2
    exit 1
fi)
grep -F "Shalimar array index out of bounds" "$BUILD_DIR/ir_native_array_bounds.err" >/dev/null
"$COMPILER" "$ROOT/tests/ir_native_array_negative_index.shl" --emit-c "$BUILD_DIR/ir_native_array_negative_index.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_array_negative_index.c" -lm -o "$BUILD_DIR/ir_native_array_negative_index"
(ulimit -c 0; if "$BUILD_DIR/ir_native_array_negative_index" >"$BUILD_DIR/ir_native_array_negative_index.out" 2>"$BUILD_DIR/ir_native_array_negative_index.err"; then
    echo "IR-native negative array index test unexpectedly succeeded" >&2
    exit 1
fi)
grep -F "Shalimar array index out of bounds" "$BUILD_DIR/ir_native_array_negative_index.err" >/dev/null
# IR-native multidimensional arrays use flat row-major storage, validate each
# dimension independently, and preserve nested initializer zero-fill semantics.
"$COMPILER" "$ROOT/tests/ir_native_multidimensional.shl" --emit-c "$BUILD_DIR/ir_native_multidimensional_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_multidimensional.shl" --emit-c "$BUILD_DIR/ir_native_multidimensional_ir.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic "$BUILD_DIR/ir_native_multidimensional_ast.c" -lm -o "$BUILD_DIR/ir_native_multidimensional_ast"
cc -std=c99 -Wall -Wextra -pedantic "$BUILD_DIR/ir_native_multidimensional_ir.c" -lm -o "$BUILD_DIR/ir_native_multidimensional_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_multidimensional_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_multidimensional_ir" | sed 's/ *$//')
EXPECTED=$(printf '1 0 3\n4 0 0\n9\n7 8 0\n6 0 0')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST multidimensional-array output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native multidimensional-array output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/array_bounds_multidimensional.shl" --emit-c "$BUILD_DIR/ir_native_multidimensional_bounds.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_multidimensional_bounds.c" -lm -o "$BUILD_DIR/ir_native_multidimensional_bounds"
(ulimit -c 0; if "$BUILD_DIR/ir_native_multidimensional_bounds" >"$BUILD_DIR/ir_native_multidimensional_bounds.out" 2>"$BUILD_DIR/ir_native_multidimensional_bounds.err"; then
    echo "IR-native multidimensional upper-bound test unexpectedly succeeded" >&2
    exit 1
fi)
grep -F "Shalimar array index out of bounds" "$BUILD_DIR/ir_native_multidimensional_bounds.err" >/dev/null
"$COMPILER" "$ROOT/tests/ir_native_multidimensional_negative_column.shl" --emit-c "$BUILD_DIR/ir_native_multidimensional_negative_column.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_multidimensional_negative_column.c" -lm -o "$BUILD_DIR/ir_native_multidimensional_negative_column"
(ulimit -c 0; if "$BUILD_DIR/ir_native_multidimensional_negative_column" >"$BUILD_DIR/ir_native_multidimensional_negative_column.out" 2>"$BUILD_DIR/ir_native_multidimensional_negative_column.err"; then
    echo "IR-native multidimensional negative-index test unexpectedly succeeded" >&2
    exit 1
fi)
grep -F "Shalimar array index out of bounds" "$BUILD_DIR/ir_native_multidimensional_negative_column.err" >/dev/null

# IR-native array parameters carry runtime dimensions, can be forwarded to
# another function, and support checked reads/writes, reassignment, and metadata.
"$COMPILER" "$ROOT/tests/ir_native_array_parameters.shl" --emit-c "$BUILD_DIR/ir_native_array_parameters_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_array_parameters.shl" --emit-c "$BUILD_DIR/ir_native_array_parameters_ir.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic "$BUILD_DIR/ir_native_array_parameters_ast.c" -lm -o "$BUILD_DIR/ir_native_array_parameters_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_array_parameters_ir.c" -lm -o "$BUILD_DIR/ir_native_array_parameters_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_array_parameters_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_array_parameters_ir" | sed 's/ *$//')
EXPECTED=$(printf '9 8 0 0\n19\n7 8 0 6 0 0\n11')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST array-parameter output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native array-parameter output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_array_parameter_bounds.shl" --emit-c "$BUILD_DIR/ir_native_array_parameter_bounds.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_array_parameter_bounds.c" -lm -o "$BUILD_DIR/ir_native_array_parameter_bounds"
(ulimit -c 0; if "$BUILD_DIR/ir_native_array_parameter_bounds" >"$BUILD_DIR/ir_native_array_parameter_bounds.out" 2>"$BUILD_DIR/ir_native_array_parameter_bounds.err"; then
    echo "IR-native array-parameter bounds test unexpectedly succeeded" >&2
    exit 1
fi)
grep -F "Shalimar array index out of bounds" "$BUILD_DIR/ir_native_array_parameter_bounds.err" >/dev/null
"$COMPILER" "$ROOT/tests/ir_native_array_parameter_reassignment_too_large.shl" --emit-c "$BUILD_DIR/ir_native_array_parameter_reassignment_too_large.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_array_parameter_reassignment_too_large.c" -lm -o "$BUILD_DIR/ir_native_array_parameter_reassignment_too_large"
(ulimit -c 0; if "$BUILD_DIR/ir_native_array_parameter_reassignment_too_large" >"$BUILD_DIR/ir_native_array_parameter_reassignment_too_large.out" 2>"$BUILD_DIR/ir_native_array_parameter_reassignment_too_large.err"; then
    echo "IR-native oversized array-parameter reassignment unexpectedly succeeded" >&2
    exit 1
fi)
grep -F "Shalimar array initializer exceeds runtime dimension" "$BUILD_DIR/ir_native_array_parameter_reassignment_too_large.err" >/dev/null

# IR-native scalar globals: zero defaults, source-order initialization,
# initializer calls, and reads/writes from functions must match the AST backend.
"$COMPILER" "$ROOT/tests/ir_native_globals.shl" --emit-c "$BUILD_DIR/ir_native_globals_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_globals.shl" --emit-c "$BUILD_DIR/ir_native_globals_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_globals_ast.c" -lm -o "$BUILD_DIR/ir_native_globals_ast"
cc -std=c99 "$BUILD_DIR/ir_native_globals_ir.c" -lm -o "$BUILD_DIR/ir_native_globals_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_globals_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_globals_ir" | sed 's/ *$//')
EXPECTED=$(printf '0 8 21\n2 24')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST scalar-global output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native scalar-global output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_globals.shl" --object "$BUILD_DIR/ir_native_globals.o" --release --ir-native >/dev/null
# IR-native fixed-size global arrays: source-order initialization, array metadata,
# function forwarding/mutation, and whole-array reassignment must match AST output.
"$COMPILER" "$ROOT/tests/ir_native_global_arrays.shl" --emit-c "$BUILD_DIR/ir_native_global_arrays_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_global_arrays.shl" --emit-c "$BUILD_DIR/ir_native_global_arrays_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_global_arrays_ast.c" -lm -o "$BUILD_DIR/ir_native_global_arrays_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_global_arrays_ir.c" -o "$BUILD_DIR/ir_native_global_arrays_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_global_arrays_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_global_arrays_ir" | sed 's/ *$//')
EXPECTED=$(printf '4 0 3 0\n1 2 0 4 0 0\n0 0\n4 -1 4 4\n15\n9 8 0 0\n7 8 0 6 0 0')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST global-array output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native global-array output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_global_arrays.shl" --object "$BUILD_DIR/ir_native_global_arrays.o" --release --ir-native >/dev/null
"$COMPILER" "$ROOT/tests/ir_native_global_array_bounds.shl" --emit-c "$BUILD_DIR/ir_native_global_array_bounds.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_global_array_bounds.c" -o "$BUILD_DIR/ir_native_global_array_bounds"
if "$BUILD_DIR/ir_native_global_array_bounds" >"$BUILD_DIR/ir_native_global_array_bounds.log" 2>&1; then
    echo "IR-native global array upper-bound access unexpectedly succeeded" >&2
    exit 1
fi
grep -F "Shalimar array index out of bounds" "$BUILD_DIR/ir_native_global_array_bounds.log" >/dev/null

# Release-only Shalimar constant folding; debug builds retain source expressions.
"$COMPILER" "$ROOT/tests/release_constant_folding.shl" --emit-c "$BUILD_DIR/release_constant_folding.c" --release
grep -E "folded[[:space:]]*=[[:space:]]*17" "$BUILD_DIR/release_constant_folding.c" >/dev/null
grep -E "compared[[:space:]]*=[[:space:]]*1" "$BUILD_DIR/release_constant_folding.c" >/dev/null
if grep -F "2 + 3" "$BUILD_DIR/release_constant_folding.c" >/dev/null; then
    echo "Release constant-folding pass left a foldable expression in generated C" >&2
    exit 1
fi
cc -std=c99 "$BUILD_DIR/release_constant_folding.c" -lm -o "$BUILD_DIR/release_constant_folding"
OUTPUT=$("$BUILD_DIR/release_constant_folding" | sed 's/ *$//')
[ "$OUTPUT" = "17 1" ] || {
    echo "unexpected release constant-folding output: $OUTPUT" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/release_constant_folding.shl" --emit-c "$BUILD_DIR/debug_constant_folding.c" --debug
grep -F "__shalimar_int_add((int32_t)(2), (int32_t)(3))" "$BUILD_DIR/debug_constant_folding.c" >/dev/null

# IR-native scalar real and char support: typed locals/globals, arguments,
# returns, arithmetic, and type-aware printing must match the AST backend.
"$COMPILER" "$ROOT/tests/ir_native_real_char.shl" --emit-c "$BUILD_DIR/ir_native_real_char_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_real_char.shl" --emit-c "$BUILD_DIR/ir_native_real_char_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_real_char_ast.c" -lm -o "$BUILD_DIR/ir_native_real_char_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_real_char_ir.c" -lm -o "$BUILD_DIR/ir_native_real_char_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_real_char_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_real_char_ir" | sed 's/ *$//')
EXPECTED=$(printf '7.5000000 B\n3.7500000')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST real/char output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native real/char output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_real_char.shl" --object "$BUILD_DIR/ir_native_real_char.o" --release --ir-native >/dev/null

# IR-native string literal output and print-precision directives must match the AST backend.
"$COMPILER" "$ROOT/tests/ir_native_strings_precision.shl" --emit-c "$BUILD_DIR/ir_native_strings_precision_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_strings_precision.shl" --emit-c "$BUILD_DIR/ir_native_strings_precision_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_strings_precision_ast.c" -lm -o "$BUILD_DIR/ir_native_strings_precision_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_strings_precision_ir.c" -lm -o "$BUILD_DIR/ir_native_strings_precision_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_strings_precision_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_strings_precision_ir" | sed 's/ *$//')
EXPECTED=$(printf 'Shalimar 2.5000000\n0.333\n3\n0.3333333')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST string/precision output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native string/precision output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_strings_precision.shl" --object "$BUILD_DIR/ir_native_strings_precision.o" --release --ir-native >/dev/null

# IR-native char[] storage, string assignment/truncation, output, and parameter forwarding.
"$COMPILER" "$ROOT/tests/ir_native_string_parameters.shl" --emit-c "$BUILD_DIR/ir_native_string_parameters_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_string_parameters.shl" --emit-c "$BUILD_DIR/ir_native_string_parameters_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_string_parameters_ast.c" -lm -o "$BUILD_DIR/ir_native_string_parameters_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_string_parameters_ir.c" -lm -o "$BUILD_DIR/ir_native_string_parameters_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_string_parameters_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_string_parameters_ir" | sed 's/ *$//')
EXPECTED=$(printf 'hello abc\nh\nHello\nHello 12\nHello 12')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST string-parameter output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native string-parameter output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_string_assignment.shl" --emit-c "$BUILD_DIR/ir_native_string_assignment_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_string_assignment.shl" --emit-c "$BUILD_DIR/ir_native_string_assignment_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_string_assignment_ast.c" -lm -o "$BUILD_DIR/ir_native_string_assignment_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_string_assignment_ir.c" -lm -o "$BUILD_DIR/ir_native_string_assignment_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_string_assignment_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_string_assignment_ir" | sed 's/ *$//')
EXPECTED=$(printf 'Shalimar abc\nlon\nlon\nchanged\nchanged\ncha\ncha')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST string-assignment output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native string-assignment output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_string_assignment.shl" --object "$BUILD_DIR/ir_native_string_assignment.o" --release --ir-native >/dev/null

# String comparisons use content semantics; char[] append is capacity-bounded and alias-safe.
"$COMPILER" "$ROOT/tests/ir_native_string_compare_append.shl" --emit-c "$BUILD_DIR/ir_native_string_compare_append_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_string_compare_append.shl" --emit-c "$BUILD_DIR/ir_native_string_compare_append_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_string_compare_append_ast.c" -lm -o "$BUILD_DIR/ir_native_string_compare_append_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_string_compare_append_ir.c" -lm -o "$BUILD_DIR/ir_native_string_compare_append_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_string_compare_append_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_string_compare_append_ir" | sed 's/ *$//')
EXPECTED=$(printf '1 1 1 1\n1 1\napple pie\napple piexx\nabab\n1 1')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST string-comparison/append output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native string-comparison/append output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_string_compare_append.shl" --object "$BUILD_DIR/ir_native_string_compare_append.o" --release --ir-native >/dev/null

# Dynamic string concatenation expressions allocate a result and can be printed,
# compared by content, or copied into a bounded char[] destination.
"$COMPILER" "$ROOT/tests/ir_native_string_concatenation.shl" --emit-c "$BUILD_DIR/ir_native_string_concatenation_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_string_concatenation.shl" --emit-c "$BUILD_DIR/ir_native_string_concatenation_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_string_concatenation_ast.c" -lm -o "$BUILD_DIR/ir_native_string_concatenation_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_string_concatenation_ir.c" -lm -o "$BUILD_DIR/ir_native_string_concatenation_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_string_concatenation_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_string_concatenation_ir" | sed 's/ *$//')
EXPECTED=$(printf 'Shalimar\nShalimar\nShalimar\n1\n1\n1 1 1\ngoin')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST string-concatenation output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$AST_OUTPUT" ] || {
    echo "IR-native string-concatenation output differs from AST backend: AST=[$AST_OUTPUT], IR=[$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_string_concatenation.shl" --object "$BUILD_DIR/ir_native_string_concatenation.o" --release --ir-native >/dev/null

# String-returning functions use an owned-copy ABI in both backends.
"$COMPILER" "$ROOT/tests/string_return.shl" --emit-c "$BUILD_DIR/string_return_ast.c" --release
"$COMPILER" "$ROOT/tests/string_return.shl" --emit-c "$BUILD_DIR/string_return_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/string_return_ast.c" -lm -o "$BUILD_DIR/string_return_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/string_return_ir.c" -lm -o "$BUILD_DIR/string_return_ir"
AST_OUTPUT=$("$BUILD_DIR/string_return_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/string_return_ir" | sed 's/ *$//')
EXPECTED=$(printf 'Shalimar\nShalimar Compiler\n1\nlocal text')
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST string-return output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected IR-native string-return output: [$IR_OUTPUT]" >&2
    exit 1
}

# Global char[] buffers are allocated statically, initialized in source order,
# and remain mutable/visible across function calls in the IR-native backend.
"$COMPILER" "$ROOT/tests/ir_native_global_strings.shl" --emit-c "$BUILD_DIR/ir_native_global_strings.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_global_strings.c" -lm -o "$BUILD_DIR/ir_native_global_strings"
OUTPUT=$("$BUILD_DIR/ir_native_global_strings" | sed 's/ *$//')
EXPECTED=$(printf 'hello 8\nabc 4\nupdated hello!\nhello!\nHpdated Xello!')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected IR-native global-string output: [$OUTPUT]" >&2
    exit 1
}

"$COMPILER" "$ROOT/tests/ir_native_global_strings.shl" --object "$BUILD_DIR/ir_native_global_strings.o" --release --ir-native >/dev/null

# Global string initializers support earlier globals, concatenation, copying,
# truncation, and calls to char[]-returning functions.
"$COMPILER" "$ROOT/tests/ir_native_global_string_initializers.shl" --emit-c "$BUILD_DIR/ir_native_global_string_initializers.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_global_string_initializers.c" -lm -o "$BUILD_DIR/ir_native_global_string_initializers"
OUTPUT=$("$BUILD_DIR/ir_native_global_string_initializers" | sed 's/ *$//')
EXPECTED=$(printf 'Shali\nShalimar\nShalimar\nShali!\nShali\nShalimar Compiler\n1')
[ "$OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected IR-native global-string-initializer output: [$OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_global_string_initializers.shl" --object "$BUILD_DIR/ir_native_global_string_initializers.o" --release --ir-native >/dev/null

# Scalar reference parameters in the IR-native backend use pointer parameters
# and copy values back on explicit returns and fallthrough. Release optimization
# must not replace addressable call arguments with constants before a call.
"$COMPILER" "$ROOT/tests/ir_native_scalar_reference.shl" --emit-c "$BUILD_DIR/ir_native_scalar_reference_ast.c" --release
"$COMPILER" "$ROOT/tests/ir_native_scalar_reference.shl" --emit-c "$BUILD_DIR/ir_native_scalar_reference_ir.c" --release --ir-native
cc -std=c99 "$BUILD_DIR/ir_native_scalar_reference_ast.c" -lm -o "$BUILD_DIR/ir_native_scalar_reference_ast"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_scalar_reference_ir.c" -lm -o "$BUILD_DIR/ir_native_scalar_reference_ir"
AST_OUTPUT=$("$BUILD_DIR/ir_native_scalar_reference_ast" | sed 's/ *$//')
IR_OUTPUT=$("$BUILD_DIR/ir_native_scalar_reference_ir" | sed 's/ *$//')
EXPECTED='11 6 3.0000000 B'
[ "$AST_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected AST scalar-reference output: [$AST_OUTPUT]" >&2
    exit 1
}
[ "$IR_OUTPUT" = "$EXPECTED" ] || {
    echo "unexpected IR-native scalar-reference output: [$IR_OUTPUT]" >&2
    exit 1
}
"$COMPILER" "$ROOT/tests/ir_native_scalar_reference.shl" --object "$BUILD_DIR/ir_native_scalar_reference.o" --release --ir-native >/dev/null

# Indexed int-array elements can be passed to scalar reference parameters. Both
# rank-one and rank-two addresses retain the normal runtime bounds checks.
"$COMPILER" "$ROOT/tests/scalar_reference_element.shl" --emit-c "$BUILD_DIR/ir_native_scalar_reference_element.c" --release --ir-native
"$COMPILER" "$ROOT/tests/ir_native_scalar_reference_multidimensional.shl" --emit-c "$BUILD_DIR/ir_native_scalar_reference_multidimensional.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_scalar_reference_element.c" -lm -o "$BUILD_DIR/ir_native_scalar_reference_element"
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_scalar_reference_multidimensional.c" -lm -o "$BUILD_DIR/ir_native_scalar_reference_multidimensional"
OUTPUT=$("$BUILD_DIR/ir_native_scalar_reference_element" | sed 's/ *$//')
[ "$OUTPUT" = "22" ] || {
    echo "unexpected IR-native rank-one element-reference output: [$OUTPUT]" >&2
    exit 1
}
OUTPUT=$("$BUILD_DIR/ir_native_scalar_reference_multidimensional" | sed 's/ *$//')
[ "$OUTPUT" = "10 9" ] || {
    echo "unexpected IR-native rank-two element-reference output: [$OUTPUT]" >&2
    exit 1
}

# A char[] element cannot be passed as int32_t * directly, so the IR-native
# backend uses a checked-indexed copy-in/copy-back proxy for scalar char refs.
"$COMPILER" "$ROOT/tests/ir_native_char_reference_element.shl" --emit-c "$BUILD_DIR/ir_native_char_reference_element.c" --release --ir-native
cc -std=c99 -Wall -Wextra -pedantic -Werror "$BUILD_DIR/ir_native_char_reference_element.c" -lm -o "$BUILD_DIR/ir_native_char_reference_element"
OUTPUT=$("$BUILD_DIR/ir_native_char_reference_element" | sed 's/ *$//')
[ "$OUTPUT" = "2 dat" ] || {
    echo "unexpected IR-native char-element-reference output: [$OUTPUT]" >&2
    exit 1
}

echo "Smoke tests passed (checked integer arithmetic/division/remainder/conversions in AST and IR-native; expression precedence/power/conversion parity; IR module split; IR-native scalar, int-array-element, and char-string-element reference parameters; string-returning functions in AST and IR-native backends; IR-native global char[] storage and expression/function initializers; char[] variables, assignment/truncation, content comparisons, bounded append, dynamic concatenation, string output, and char[] parameter forwarding)."


# Rev14: returned char[] owns its value; bounded assignment truncates and terminates.
for mode in debug release; do
    flag="--$mode"
    for backend in ast ir; do
        extra=
        if [ "$backend" = ir ]; then extra=--ir-native; fi
        "$COMPILER" "$ROOT/tests/rev14_char_return_contract.shl" --emit-c "$BUILD_DIR/rev14_${mode}_${backend}.c" "$flag" $extra
        cc -std=c99 "$BUILD_DIR/rev14_${mode}_${backend}.c" -lm -o "$BUILD_DIR/rev14_${mode}_${backend}"
        OUTPUT=$("$BUILD_DIR/rev14_${mode}_${backend}" | sed 's/ *$//')
        EXPECTED=$(printf 'abcd\nabcdef!\nShalimar')
        [ "$OUTPUT" = "$EXPECTED" ] || {
            echo "rev14 char return contract failed ($mode/$backend): [$OUTPUT]" >&2
            exit 1
        }
    done
done
