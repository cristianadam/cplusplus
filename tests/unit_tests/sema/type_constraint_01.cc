// RUN: %cxx -verify -fsyntax-only %s

template <typename T>
concept Integral = __is_integral(T);

namespace N {
template <typename T>
concept Small = sizeof(T) <= 4;
}  // namespace N

// A constrained placeholder, with the concept named plainly and with a scope
// in front of it.
Integral auto one() { return 1; }
N::Small auto two() { return 'c'; }

// A constrained template parameter, and a requires clause written in each of
// the two places one can be.
template <Integral T>
auto three(T t) -> T {
  return t;
}

template <typename T>
  requires Integral<T>
auto four(T t) -> T {
  return t;
}

template <typename T>
auto five(T t) -> T
  requires N::Small<T>
{
  return t;
}

auto main() -> int {
  static_assert(__is_same(decltype(one()), int));
  static_assert(__is_same(decltype(two()), char));
  static_assert(__is_same(decltype(three(1)), int));
  static_assert(__is_same(decltype(four(1)), int));
  static_assert(__is_same(decltype(five('c')), char));
  return 0;
}
