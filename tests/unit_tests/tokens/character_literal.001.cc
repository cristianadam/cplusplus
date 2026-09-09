// RUN: %cxx -verify -dump-tokens %s -o - | %filecheck %s

// clang-format off

'c'
// CHECK: CHARACTER_LITERAL ''c'' [start-of-line]

L'c'
// CHECK: CHARACTER_LITERAL 'L'c'' [start-of-line]

u8'c'
// CHECK: CHARACTER_LITERAL 'u8'c'' [start-of-line]

// The ud-suffix is part of the literal, as it is for integer and floating
// point literals.

'c'_X
// CHECK: CHARACTER_LITERAL ''c'_X' [start-of-line]

L'c'_wc
// CHECK: CHARACTER_LITERAL 'L'c'_wc' [start-of-line]
