#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/preprocessor.h>
#include <cxx/memory_layout.h>
#include <cxx/preprocessor_delegate.h>
#include <cxx/symbols.h>
#include <cxx/translation_unit.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace cxx;

namespace {

struct DefinedMacro {
  std::string name;
  std::string body;
  std::vector<std::string> parameters;
  bool functionLike = false;
  bool variadic = false;
};

struct UsedMacro {
  std::string name;
  bool expanded = false;
  std::vector<std::string> arguments;
};

// Records everything, and resolves the ranges it is given back to the text
// they cover, which is the point of reporting ranges rather than tokens.
class RecordingDelegate final : public PreprocessorDelegate {
 public:
  explicit RecordingDelegate(Preprocessor& preprocessor)
      : preprocessor_(preprocessor) {}

  std::vector<DefinedMacro> defined;
  std::vector<std::string> undefined;
  std::vector<UsedMacro> used;
  std::vector<std::string> undefinedUses;
  std::vector<std::string> skipped;
  std::vector<std::string> guards;
  std::vector<std::string> pragmas;

  void macroDefined(const MacroInfo& macro) override {
    defined.push_back({std::string(macro.name), std::string(macro.body),
                       {macro.parameters.begin(), macro.parameters.end()},
                       macro.isFunctionLike, macro.isVariadic});
  }

  void macroUndefined(const MacroInfo& macro, PreprocessorRange) override {
    undefined.emplace_back(macro.name);
  }

  void macroUsed(const MacroUse& use) override {
    UsedMacro entry{std::string(use.macro->name), use.expanded, {}};
    for (const auto& argument : use.arguments)
      entry.arguments.push_back(textOf(argument));
    // The range has to point at the name that was used.
    EXPECT_EQ(textOf(use.range), entry.name);
    used.push_back(std::move(entry));
  }

  void undefinedMacroUsed(std::string_view name, PreprocessorRange) override {
    undefinedUses.emplace_back(name);
  }

  void regionSkipped(PreprocessorRange range) override {
    skipped.push_back(textOf(range));
  }

  void includeGuardFound(std::uint32_t, std::string_view macroName) override {
    guards.emplace_back(macroName);
  }

  void pragmaDirective(PreprocessorRange range) override {
    pragmas.push_back(textOf(range));
  }

 private:
  [[nodiscard]] auto textOf(PreprocessorRange range) const -> std::string {
    if (!range) return {};
    const std::string& source = preprocessor_.source(range.fileId);
    if (range.end() > source.size()) return {};
    return source.substr(range.offset, range.length);
  }

  Preprocessor& preprocessor_;
};

class SilentDiagnostics final : public DiagnosticsClient {
 public:
  void report(const Diagnostic&) override {}
};

// Preprocesses a snippet with a delegate attached and hands the delegate back.
class Preprocessed {
 public:
  explicit Preprocessed(const std::string& source)
      : preprocessor_(&control_, &diagnostics_), delegate_(preprocessor_) {
    preprocessor_.setCanResolveFiles(false);
    preprocessor_.setPreprocessorDelegate(&delegate_);
    preprocessor_.preprocess(source, "test.cc", tokens_);
  }

  auto operator->() -> RecordingDelegate* { return &delegate_; }

 private:
  Control control_;
  SilentDiagnostics diagnostics_;
  Preprocessor preprocessor_;
  RecordingDelegate delegate_;
  std::vector<Token> tokens_;
};

}  // namespace

TEST(PreprocessorDelegate, reports_an_object_macro_definition) {
  Preprocessed pp("#define ANSWER 42\n");

  ASSERT_EQ(pp->defined.size(), 1);
  EXPECT_EQ(pp->defined[0].name, "ANSWER");
  EXPECT_EQ(pp->defined[0].body, "42");
  EXPECT_FALSE(pp->defined[0].functionLike);
}

TEST(PreprocessorDelegate, reports_a_function_macro_definition) {
  Preprocessed pp("#define ADD(a, b) a + b\n");

  ASSERT_EQ(pp->defined.size(), 1);
  EXPECT_EQ(pp->defined[0].name, "ADD");
  EXPECT_EQ(pp->defined[0].body, "a + b");
  EXPECT_TRUE(pp->defined[0].functionLike);
  EXPECT_EQ(pp->defined[0].parameters, (std::vector<std::string>{"a", "b"}));
}

TEST(PreprocessorDelegate, reports_a_variadic_macro_definition) {
  Preprocessed pp("#define LOG(fmt, ...) print(fmt, __VA_ARGS__)\n");

  ASSERT_EQ(pp->defined.size(), 1);
  EXPECT_TRUE(pp->defined[0].variadic);
  EXPECT_EQ(pp->defined[0].parameters, (std::vector<std::string>{"fmt"}));
}

TEST(PreprocessorDelegate, reports_an_undef) {
  Preprocessed pp("#define A 1\n#undef A\n");

  EXPECT_EQ(pp->undefined, (std::vector<std::string>{"A"}));
}

TEST(PreprocessorDelegate, does_not_report_an_undef_of_nothing) {
  Preprocessed pp("#undef NEVER_DEFINED\n");

  EXPECT_TRUE(pp->undefined.empty());
}

TEST(PreprocessorDelegate, reports_an_object_macro_being_expanded) {
  Preprocessed pp("#define ANSWER 42\nint x = ANSWER;\n");

  ASSERT_EQ(pp->used.size(), 1);
  EXPECT_EQ(pp->used[0].name, "ANSWER");
  EXPECT_TRUE(pp->used[0].expanded);
}

TEST(PreprocessorDelegate, reports_the_arguments_of_an_expansion) {
  Preprocessed pp("#define ADD(a, b) a + b\nint x = ADD(1 + 2, y);\n");

  ASSERT_EQ(pp->used.size(), 1);
  EXPECT_EQ(pp->used[0].name, "ADD");
  EXPECT_TRUE(pp->used[0].expanded);
  EXPECT_EQ(pp->used[0].arguments, (std::vector<std::string>{"1 + 2", "y"}));
}

TEST(PreprocessorDelegate, reports_a_macro_that_is_only_asked_about) {
  Preprocessed pp("#define A 1\n#ifdef A\n#endif\n");

  ASSERT_EQ(pp->used.size(), 1);
  EXPECT_EQ(pp->used[0].name, "A");
  EXPECT_FALSE(pp->used[0].expanded);
}

TEST(PreprocessorDelegate, reports_a_name_that_is_not_a_macro) {
  Preprocessed pp("#ifdef NOT_A_MACRO\n#endif\n");

  EXPECT_EQ(pp->undefinedUses, (std::vector<std::string>{"NOT_A_MACRO"}));
  EXPECT_TRUE(pp->used.empty());
}

TEST(PreprocessorDelegate, reports_the_region_an_if_left_out) {
  Preprocessed pp("#if 0\nint skipped;\n#endif\nint kept;\n");

  ASSERT_EQ(pp->skipped.size(), 1);
  EXPECT_EQ(pp->skipped[0], "\nint skipped;\n");
}

TEST(PreprocessorDelegate, reports_the_arm_of_an_else_that_was_not_taken) {
  Preprocessed pp("#if 1\nint taken;\n#else\nint other;\n#endif\n");

  ASSERT_EQ(pp->skipped.size(), 1);
  EXPECT_EQ(pp->skipped[0], "\nint other;\n");
}

TEST(PreprocessorDelegate, reports_one_region_for_a_nested_if) {
  Preprocessed pp("#if 0\n#if 1\nint a;\n#endif\nint b;\n#endif\n");

  ASSERT_EQ(pp->skipped.size(), 1);
  EXPECT_EQ(pp->skipped[0], "\n#if 1\nint a;\n#endif\nint b;\n");
}

TEST(PreprocessorDelegate, reports_nothing_skipped_when_nothing_is) {
  Preprocessed pp("#if 1\nint a;\n#endif\n");

  EXPECT_TRUE(pp->skipped.empty());
}

TEST(PreprocessorDelegate, reports_a_pragma) {
  Preprocessed pp("#pragma once\n");

  EXPECT_EQ(pp->pragmas, (std::vector<std::string>{"once"}));
}

// A tool that keeps one translation unit per file preprocesses a header on
// its own, so the guard has to be found there as well as on the way into an
// include. Without it, such a tool sees a file that asks about a macro it
// then defines, and cannot tell that the answer was its own doing.
TEST(PreprocessorDelegate, reports_the_include_guard_of_the_main_file) {
  Preprocessed pp(
      "#ifndef GUARDED_H\n#define GUARDED_H\nint fromHeader;\n#endif\n");

  EXPECT_EQ(pp->guards, (std::vector<std::string>{"GUARDED_H"}));
}

TEST(PreprocessorDelegate, reports_no_guard_for_an_unguarded_file) {
  Preprocessed pp("#ifndef SOMETHING\nint a;\n#endif\nint b;\n");

  EXPECT_TRUE(pp->guards.empty());
}

// This one drives the preprocessor's own state machine and answers the
// request for the content, which is how a header is reached as an include.
TEST(PreprocessorDelegate, reports_an_include_guard) {
  Control control;
  SilentDiagnostics diagnostics;
  Preprocessor preprocessor(&control, &diagnostics);
  RecordingDelegate delegate(preprocessor);

  preprocessor.setCanResolveFiles(false);
  preprocessor.setPreprocessorDelegate(&delegate);

  struct Answering {
    bool done = false;
    explicit operator bool() const { return !done; }

    void operator()(const ProcessingComplete&) { done = true; }
    void operator()(const CanContinuePreprocessing&) {}
    void operator()(const EnteringFile&) {}
    void operator()(const LeavingFile&) {}
    void operator()(const PendingInclude& state) {
      state.resolveWith(std::string("guarded.h"));
    }
    void operator()(const PendingHasIncludes& state) {
      for (const auto& request : state.requests) request.setExists(true);
    }
    void operator()(const PendingFileContent& request) {
      request.setContent(std::string("#ifndef GUARDED_H\n#define GUARDED_H\n"
                                     "int fromHeader;\n#endif\n"));
    }
  };

  std::vector<Token> tokens;
  preprocessor.beginPreprocessing("#include \"guarded.h\"\nint main() {}\n",
                                  "test.cc", tokens);
  Answering answering;
  while (answering)
    std::visit(answering, preprocessor.continuePreprocessing(tokens));
  preprocessor.endPreprocessing(tokens);

  EXPECT_EQ(delegate.guards, (std::vector<std::string>{"GUARDED_H"}));
}

// The tokens say which of them a macro produced, which the delegate cannot:
// it reports the invocation, not each token that came out of it.

namespace {

// The kinds of the output tokens, with a marker for each flag. Kinds rather
// than text, because the text at an expanded token's offset is the invocation
// rather than what the macro replaced it with -- which is the point of the
// offset, and what the last test here checks.
auto tokenSummary(const std::string& source) -> std::vector<std::string> {
  Control control;
  SilentDiagnostics diagnostics;
  Preprocessor preprocessor(&control, &diagnostics);
  preprocessor.setCanResolveFiles(false);

  std::vector<Token> tokens;
  preprocessor.preprocess(source, "test.cc", tokens);

  std::vector<std::string> result;
  for (const Token& token : tokens) {
    if (token.is(TokenKind::T_EOF_SYMBOL)) continue;
    if (token.fileId() != std::uint32_t(preprocessor.mainSourceFileId())) continue;
    std::string entry = Token::spell(token.kind());
    if (token.macroExpanded()) entry += " expanded";
    if (token.macroGenerated()) entry += " generated";
    result.push_back(entry);
  }
  return result;
}

}  // namespace

TEST(PreprocessorTokens, marks_nothing_when_no_macro_is_involved) {
  EXPECT_EQ(tokenSummary("int x;\n"),
            (std::vector<std::string>{"int", "<identifier>", ";"}));
}

TEST(PreprocessorTokens, marks_what_an_object_macro_produced) {
  EXPECT_EQ(tokenSummary("#define ANSWER 42\nint x = ANSWER;\n"),
            (std::vector<std::string>{"int", "<identifier>", "=",
                                      "<integer_literal> expanded generated",
                                      ";"}));
}

TEST(PreprocessorTokens, tells_a_macro_body_from_an_argument) {
  // a and b come from what the caller wrote, the + comes from the body.
  EXPECT_EQ(tokenSummary("#define ADD(a, b) a + b\nint x = ADD(p, q);\n"),
            (std::vector<std::string>{"int", "<identifier>", "=",
                                      "<identifier> expanded",
                                      "+ expanded generated",
                                      "<identifier> expanded", ";"}));
}

TEST(PreprocessorTokens, a_generated_token_points_at_the_invocation) {
  Control control;
  SilentDiagnostics diagnostics;
  Preprocessor preprocessor(&control, &diagnostics);
  preprocessor.setCanResolveFiles(false);

  const std::string source = "#define ANSWER 42\nint x = ANSWER;\n";
  std::vector<Token> tokens;
  preprocessor.preprocess(source, "test.cc", tokens);

  for (const Token& token : tokens) {
    if (!token.macroGenerated()) continue;
    // Not at the 42 in the #define: at the ANSWER that asked for it.
    EXPECT_EQ(source.substr(token.offset(), token.length()), "ANSWER");
  }
}

TEST(PreprocessorTokens, an_argument_points_at_what_the_caller_wrote) {
  Control control;
  SilentDiagnostics diagnostics;
  Preprocessor preprocessor(&control, &diagnostics);
  preprocessor.setCanResolveFiles(false);

  const std::string source = "#define ID(x) x\nint y = ID(written);\n";
  std::vector<Token> tokens;
  preprocessor.preprocess(source, "test.cc", tokens);

  int seen = 0;
  for (const Token& token : tokens) {
    if (!token.macroExpanded() || token.macroGenerated()) continue;
    ++seen;
    EXPECT_EQ(source.substr(token.offset(), token.length()), "written");
  }
  EXPECT_EQ(seen, 1);
}

TEST(PreprocessorDelegate, reports_nothing_without_a_delegate) {
  // The whole point of the default being off: none of this is worked out
  // unless someone asks for it. Nothing to assert but that it still runs.
  Control control;
  SilentDiagnostics diagnostics;
  Preprocessor preprocessor(&control, &diagnostics);
  preprocessor.setCanResolveFiles(false);

  std::vector<Token> tokens;
  preprocessor.preprocess("#define A 1\nint x = A;\n", "test.cc", tokens);

  EXPECT_EQ(preprocessor.preprocessorDelegate(), nullptr);
}

// A scope says how far it reaches, which is what makes it possible to ask
// which scope a place in the text is in.

namespace {

// A translation unit that can be parsed without reference to the host.
struct MemoryLayoutFixture {
  MemoryLayout layout{64};
  SilentDiagnostics diagnostics;
  TranslationUnit unit{&diagnostics};

  MemoryLayoutFixture() {
    unit.control()->setMemoryLayout(&layout);
    unit.preprocessor()->setCanResolveFiles(false);
  }
};

// The innermost scope written around the token at the given index, by name.
auto innermostScopeAt(TranslationUnit& unit, unsigned tokenIndex) -> std::string {
  const SourceLocation loc{tokenIndex};

  std::string found;
  const auto consider = [&](auto&& self, ScopeSymbol* inner) -> void {
    if (!inner->contains(loc)) return;
    if (inner->name()) found = to_string(inner->name());
    self(self, inner);
  };
  const auto walk = [&](auto&& self, ScopeSymbol* scope) -> void {
    for (Symbol* member : scope->members()) {
      // A function is reached through the overload set it lives in, which is
      // not itself a scope.
      if (auto* overloadSet = symbol_cast<OverloadSetSymbol>(member)) {
        for (auto* function : overloadSet->declaredFunctions()) consider(self, function);
        continue;
      }
      if (auto* inner = member->asScopeSymbol()) consider(self, inner);
    }
  };
  walk(walk, unit.globalScope());
  return found;
}

// The index of the first token whose text is the one given.
auto tokenIndexOf(TranslationUnit& unit, std::string_view text) -> unsigned {
  for (unsigned i = 1; i < unit.tokenCount(); ++i) {
    if (unit.tokenText(SourceLocation{i}) == text) return i;
  }
  return 0;
}

}  // namespace

TEST(ScopeExtent, saysWhichScopeAPlaceIsIn) {
  MemoryLayoutFixture fixture;
  auto& unit = fixture.unit;

  unit.setSource(
      "void outside() {}\n"
      "namespace N {\n"
      "struct S {\n"
      "  void m() { int inner; }\n"
      "};\n"
      "}\n",
      "test.cc");
  unit.parse({.checkTypes = true});

  EXPECT_EQ(innermostScopeAt(unit, tokenIndexOf(unit, "inner")), "m");
}

TEST(ScopeExtent, aScopeDoesNotReachPastItsEnd) {
  MemoryLayoutFixture fixture;
  auto& unit = fixture.unit;

  unit.setSource("void f() { int a; }\nint after;\n", "test.cc");
  unit.parse({.checkTypes = true});

  EXPECT_EQ(innermostScopeAt(unit, tokenIndexOf(unit, "a")), "f");
  EXPECT_EQ(innermostScopeAt(unit, tokenIndexOf(unit, "after")), "");
}

TEST(ScopeExtent, aScopeWithNoExtentContainsNothing) {
  MemoryLayoutFixture fixture;
  auto& unit = fixture.unit;

  unit.setSource("int x;\n", "test.cc");
  unit.parse({.checkTypes = true});

  // The global scope was not written anywhere, so it says nothing about what
  // it contains rather than claiming everything.
  EXPECT_FALSE(unit.globalScope()->extentBegin());
  EXPECT_FALSE(unit.globalScope()->contains(SourceLocation{1}));
}

TEST(PreprocessorDelegate, spells_out_the_body_of_a_macro_with_no_source) {
  Control control;
  SilentDiagnostics diagnostics;
  Preprocessor preprocessor(&control, &diagnostics);
  RecordingDelegate delegate(preprocessor);

  preprocessor.setCanResolveFiles(false);
  preprocessor.setPreprocessorDelegate(&delegate);

  // Defined through the API, so there is no #define anywhere to quote. The
  // body still has to say what it is, or two different values look alike.
  preprocessor.defineMacro("ANSWER 42", "");
  preprocessor.defineMacro("ADD(a, b) a + b", "");

  std::vector<Token> tokens;
  preprocessor.preprocess("int x;\n", "test.cc", tokens);

  ASSERT_EQ(delegate.defined.size(), 2);
  EXPECT_EQ(delegate.defined[0].name, "ANSWER");
  EXPECT_EQ(delegate.defined[0].body, "42");
  EXPECT_EQ(delegate.defined[1].name, "ADD");
  EXPECT_EQ(delegate.defined[1].body, "a + b");
}

// The file being read, which for an include is the file that wrote it: the
// same name means different files in different places, so a tool that
// resolves one has to know where it stands.
TEST(Preprocessor, saysWhichFileItIsReading) {
  MemoryLayout layout(64);
  DiagnosticsClient diagnosticsClient;
  TranslationUnit unit(&diagnosticsClient);
  unit.control()->setMemoryLayout(&layout);

  auto* preprocessor = unit.preprocessor();
  preprocessor->setCanResolveFiles(false);

  std::vector<std::string> askedFrom;
  std::map<std::string, std::string> files{
      {"outer.h", "#include \"inner.h\"\n"},
      {"inner.h", "int fromInner;\n"},
  };

  unit.beginPreprocessing("#include \"outer.h\"\nint fromMain;\n", "main.cpp");
  for (bool done = false; !done;) {
    std::visit(
        [&](auto&& state) {
          using T = std::decay_t<decltype(state)>;
          if constexpr (std::is_same_v<T, ProcessingComplete>) {
            done = true;
          } else if constexpr (std::is_same_v<T, PendingInclude>) {
            askedFrom.push_back(preprocessor->currentFileName());
            const auto& name =
                std::get<QuoteInclude>(state.include).fileName;
            state.resolveWith(name, false);
          } else if constexpr (std::is_same_v<T, PendingFileContent>) {
            auto it = files.find(state.fileName);
            if (it == files.end())
              state.setContent(std::nullopt);
            else
              state.setContent(it->second);
          }
        },
        unit.continuePreprocessing());
  }
  unit.endPreprocessing();

  ASSERT_EQ(askedFrom.size(), 2u);
  EXPECT_EQ(askedFrom[0], "main.cpp");
  EXPECT_EQ(askedFrom[1], "outer.h");
}

// A file that includes something which includes it back, with neither
// guarding itself. Reading it as written has no end, so there is a depth
// at which it stops and says so.
TEST(Preprocessor, stopsAnIncludeCycleThatGuardsNothing) {
  MemoryLayout layout(64);
  DiagnosticsClient diagnosticsClient;
  TranslationUnit unit(&diagnosticsClient);
  unit.control()->setMemoryLayout(&layout);

  auto* preprocessor = unit.preprocessor();
  preprocessor->setCanResolveFiles(false);

  std::map<std::string, std::string> files{
      {"a.h", "#include \"b.h\"\n"},
      {"b.h", "#include \"a.h\"\n"},
  };

  unit.beginPreprocessing("#include \"a.h\"\n", "main.cpp");
  for (bool done = false; !done;) {
    std::visit(
        [&](auto&& state) {
          using T = std::decay_t<decltype(state)>;
          if constexpr (std::is_same_v<T, ProcessingComplete>) {
            done = true;
          } else if constexpr (std::is_same_v<T, PendingInclude>) {
            const auto& name = std::get<QuoteInclude>(state.include).fileName;
            state.resolveWith(name, false);
          } else if constexpr (std::is_same_v<T, PendingFileContent>) {
            auto it = files.find(state.fileName);
            if (it == files.end())
              state.setContent(std::nullopt);
            else
              state.setContent(it->second);
          }
        },
        unit.continuePreprocessing());
  }
  unit.endPreprocessing();

  SUCCEED();  // reaching here at all is the point
}
