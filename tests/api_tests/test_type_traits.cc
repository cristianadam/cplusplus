#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/translation_unit.h>
#include <cxx/type_traits.h>
#include <cxx/types.h>
#include <gtest/gtest.h>

using namespace cxx;

// Taking the extents off an array that carries qualifiers of its own.
//
// Whether a type is an array is asked through the qualifiers -- a qualified
// array is an array -- so taking an extent off one has to see through them
// too. While it did not, the type came back unchanged and
// remove_all_extents() asked the same question of the same type for ever: a
// translation unit of Google Mock, which builds such a type, never finished
// parsing.
TEST(TypeTraits, removeAllExtentsOfAQualifiedArrayTerminates) {
  DiagnosticsClient diagnostics;
  TranslationUnit unit(&diagnostics);
  auto control = unit.control();
  auto traits = unit.typeTraits();

  const Type* constChar =
      control->getQualType(control->getCharType(), CvQualifiers::kConst);

  // const (const char[6]), which is the shape that did not end: the
  // qualifier stands outside an array whose elements are qualified already.
  const Type* array = control->getBoundedArrayType(constChar, 6);
  const Type* qualifiedArray =
      control->getQualType(array, CvQualifiers::kConst);

  ASSERT_TRUE(traits.is_array(qualifiedArray));

  // Reaching this at all is half the point; the elements keep what the
  // array was qualified with, which is the other half.
  EXPECT_EQ(traits.remove_all_extents(qualifiedArray), constChar);

  // And the plain shapes are unchanged by looking through the qualifiers.
  EXPECT_EQ(traits.remove_all_extents(array), constChar);
  EXPECT_EQ(traits.remove_all_extents(control->getBoundedArrayType(
                control->getBoundedArrayType(control->getCharType(), 2), 3)),
            control->getCharType());
  EXPECT_EQ(traits.remove_all_extents(control->getCharType()),
            control->getCharType());
}
