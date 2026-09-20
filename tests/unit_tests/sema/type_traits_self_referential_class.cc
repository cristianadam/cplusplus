// RUN: %cxx -verify -fsyntax-only %s

// A class cannot contain itself. The parser says so and records the member
// all the same, so every trait that walks a class's members meets a class it
// is already inside -- and without a guard walks round for ever until the
// stack runs out.
//
// What is asked for here is only that an answer comes back. Which answer it
// is has no meaning: the input is ill-formed and has been reported as such,
// and a walk cut short describes where it was asked from rather than the
// class. So the results are read into variables and nothing is asserted
// about them.

// expected-error@+1 {{alignment of incomplete type '::Direct d'}}
struct Direct {
  Direct d;
};

constexpr bool directIsTriviallyCopyable = __is_trivially_copyable(Direct);
constexpr bool directIsTriviallyDestructible =
    __is_trivially_destructible(Direct);
constexpr bool directIsTrivial = __is_trivial(Direct);
constexpr bool directIsPod = __is_pod(Direct);
constexpr bool directIsStandardLayout = __is_standard_layout(Direct);
constexpr bool directIsLiteral = __is_literal_type(Direct);

// The same cycle spelled through two classes rather than one.

struct Second;

// expected-error@+1 {{alignment of incomplete type '::Second second'}}
struct First {
  Second second;
};

struct Second {
  First first;
};

constexpr bool firstIsTrivial = __is_trivial(First);
constexpr bool firstIsPod = __is_pod(First);
constexpr bool firstIsLiteral = __is_literal_type(First);

// And spelled the way it is reached by accident rather than on purpose: a
// base whose member is the class deriving from it.

template <typename Derived>
// Twice: once for the template and once for the instantiation, which is
// where the member's type is known.
// expected-error@+2 {{alignment of incomplete type '::Crtp derived'}}
// expected-error@+1 {{alignment of incomplete type '::Crtp derived'}}
struct Base {
  Derived derived;
};

// expected-note@+1 {{in instantiation of template class 'Base <::Crtp>' requested here}}
struct Crtp : Base<Crtp> {};

constexpr bool crtpIsStandardLayout = __is_standard_layout(Crtp);
constexpr bool crtpIsPod = __is_pod(Crtp);
