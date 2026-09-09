#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/preprocessor.h>
#include <cxx/preprocessor_delegate.h>
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

// Include guards are only found by reading a header, so this one drives the
// preprocessor's own state machine and answers the request for the content.
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
