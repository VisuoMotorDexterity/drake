#include "drake/geometry/geometry_properties.h"

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include "drake/common/drake_copyable.h"
#include "drake/common/test_utilities/expect_no_throw.h"
#include "drake/common/test_utilities/expect_throws_message.h"
#include "drake/common/unused.h"
#include "drake/geometry/rgba.h"

namespace drake {
namespace geometry {
namespace {

using Eigen::Vector4d;

// A constructible sub-class of GeometryProperties.
class TestProperties : public GeometryProperties {
 public:
  DRAKE_DEFAULT_COPY_AND_MOVE_AND_ASSIGN(TestProperties);

  TestProperties() = default;
};

GTEST_TEST(GeometryProperties, ManagingGroups) {
  TestProperties properties;
  const std::string& group_name{"some_group"};
  // Only contains the default group.
  ASSERT_EQ(1, properties.num_groups());
  ASSERT_FALSE(properties.HasGroup(group_name));
  ASSERT_TRUE(properties.HasGroup(TestProperties::default_group_name()));

  // Add the group for the first time by adding a property.
  properties.AddProperty(group_name, "junk_value", 1);
  ASSERT_TRUE(properties.HasGroup(group_name));
  ASSERT_EQ(2, properties.num_groups());

  // Retrieve the group.
  using PropertyGroup = GeometryProperties::Group;
  const PropertyGroup& group = properties.GetPropertiesInGroup(group_name);
  EXPECT_EQ(1u, group.size());

  DRAKE_EXPECT_THROWS_MESSAGE(properties.GetPropertiesInGroup("invalid_name"),
                              std::logic_error,
                              ".*Can't retrieve properties for a group that "
                              "doesn't exist: '.*'");
}

// Tests adding properties (successfully and otherwise). Uses a call to
// GetProperty() to confirm successful add.
GTEST_TEST(GeometryProperties, AddProperty) {
  TestProperties properties;
  const std::string& group_name{"some_group"};

  // Confirm property doesn't exist.
  const std::string prop_name("some_property");
  ASSERT_FALSE(properties.HasProperty(group_name, prop_name));

  // Add the property.
  const int int_value{7};
  DRAKE_EXPECT_NO_THROW(
      properties.AddProperty(group_name, prop_name, int_value));

  // Confirm existence.
  ASSERT_TRUE(properties.HasProperty(group_name, prop_name));
  int read_value = properties.GetProperty<int>(group_name, prop_name);
  ASSERT_EQ(int_value, read_value);

  // Redundant add.
  DRAKE_EXPECT_THROWS_MESSAGE(
      properties.AddProperty(group_name, prop_name, int_value),
      std::logic_error,
      ".*Trying to add property .+ to group .+; .* name already exists");
  ASSERT_TRUE(properties.HasProperty(group_name, prop_name));
}

// Struct for the AddPropertyStruct test.
struct TestData {
  int i {};
  double d{};
  std::string s;
};

// Tests the case where the property value is a struct.
GTEST_TEST(GeometryProperties, AddPropertyStruct) {
  TestProperties properties;

  const std::string prop_name("test data");
  TestData data{1, 2., "3"};
  DRAKE_EXPECT_NO_THROW(properties.AddProperty(
      TestProperties::default_group_name(), prop_name, data));

  const TestData& read = properties.GetProperty<TestData>(
      TestProperties::default_group_name(), prop_name);
  EXPECT_EQ(data.i, read.i);
  EXPECT_EQ(data.d, read.d);
  EXPECT_EQ(data.s, read.s);
}

// Tests property access with default.
GTEST_TEST(GeometryProperties, GetPropertyOrDefault) {
  // Create one group with a single property.
  TestProperties properties;
  const std::string group_name{"some_group"};
  const double double_value{7};
  const double default_value = double_value - 1;
  const std::string prop_name("some_property");
  DRAKE_EXPECT_NO_THROW(
      properties.AddProperty(group_name, prop_name, double_value));

  // Case: default value that can be *implicitly* converted to the desired
  // type requires explicit template declaration.
  DRAKE_EXPECT_THROWS_MESSAGE(
      properties.GetPropertyOrDefault(group_name, prop_name, 3),
      std::logic_error,
      ".*The property '.*' in group '.*' exists, but is of a different type. "
      "Requested 'int', but found 'double'");
  DRAKE_EXPECT_NO_THROW(
      properties.GetPropertyOrDefault<double>(group_name, prop_name, 3));

  // Case: read an existing property.
  int read_value = properties.GetPropertyOrDefault(group_name, prop_name,
                                                   default_value);
  EXPECT_EQ(double_value, read_value);

  // Case: read from valid group, but invalid property.
  read_value = properties.GetPropertyOrDefault(group_name, "invalid_prop",
                                               default_value);
  EXPECT_EQ(default_value, read_value);

  // Case: read from invalid group.
  read_value = properties.GetPropertyOrDefault("invalid_group", "invalid_prop",
                                               default_value);
  EXPECT_EQ(default_value, read_value);

  // Case: Property exists of different type.
  DRAKE_EXPECT_THROWS_MESSAGE(
      properties.GetPropertyOrDefault(group_name, prop_name, "test"),
      std::logic_error,
      ".*The property '" + prop_name + "' in group '" + group_name + "' "
      "exists, but is of a different type. Requested 'std::string', but found "
      "'double'");

  // Using r-values as defaults; this tests both compatibility and correctness.
  properties.AddProperty("strings", "valid_string", "valid_string");
  std::string valid_value = properties.GetPropertyOrDefault(
      "strings", "valid_string", "missing");
  EXPECT_EQ("valid_string", valid_value);
  std::string default_value_return = properties.GetPropertyOrDefault(
      "strings", "invalid_string", "rvalue_string");
  EXPECT_EQ("rvalue_string", default_value_return);
}

// Tests the unsuccessful access to properties (successful access has been
// implicitly tested in the functions that added/set properties).
GTEST_TEST(GeometryProperties, GetPropertyFailure) {
  TestProperties properties;
  const std::string& group_name{"some_group"};
  const std::string prop_name("some_property");

  // Getter errors
  // Case: Asking for property from non-existent group.
  DRAKE_EXPECT_THROWS_MESSAGE(
      properties.GetProperty<int>(group_name, prop_name), std::logic_error,
      ".*Trying to read property .* from group .*. But the group does not "
      "exist.");

  // Case: Group exists, property does not.
  properties.AddProperty(group_name, prop_name + "_alt", 1);
  DRAKE_EXPECT_THROWS_MESSAGE(
      properties.GetProperty<int>(group_name, prop_name), std::logic_error,
      ".*There is no property .* in group .*.");

  // Case: Group and property exists, but property is of different type.
  DRAKE_EXPECT_NO_THROW(properties.AddProperty(group_name, prop_name, 7.0));
  DRAKE_EXPECT_THROWS_MESSAGE(
      properties.GetProperty<int>(group_name, prop_name), std::logic_error,
      ".*The property '" + prop_name + "' in group '" + group_name + "' exists"
      ", but is of a different type. Requested 'int', but found 'double'");
}

// Tests iteration through a group's properties.
GTEST_TEST(GeometryProperties, PropertyIteration) {
  TestProperties properties;
  const std::string& default_group = TestProperties::default_group_name();
  std::unordered_map<std::string, int> reference{{"prop1", 10}, {"prop2", 20}};
  for (const auto& pair : reference) {
    properties.AddProperty(default_group, pair.first, pair.second);
  }

  // Get exception for non-existent group.
  DRAKE_EXPECT_THROWS_MESSAGE(
      properties.GetPropertiesInGroup("bad group"), std::logic_error,
      ".*Can't retrieve properties for a group that doesn't exist.*");

  // Confirm that all properties have the right value and get visited.
  std::set<std::string> visited_properties;
  for (const auto& pair : properties.GetPropertiesInGroup(default_group)) {
    const std::string& name = pair.first;
    EXPECT_GT(reference.count(name), 0);
    EXPECT_EQ(reference[name],
              properties.GetProperty<int>(default_group, name));
    visited_properties.insert(name);
  }
  EXPECT_EQ(reference.size(), visited_properties.size());
}

// Confirms that derived classes *can* be copied/moved.
GTEST_TEST(GeometryProperties, CopyMoveSemantics) {
  // Populate a property set with an arbitrary set of properties. In this case,
  // they are all int-valued to facilitate comparison between property sets.
  auto make_properties = []() {
    TestProperties props;
    const std::string& default_group = TestProperties::default_group_name();
    props.AddProperty(default_group, "prop1", 1);
    props.AddProperty(default_group, "prop2", 2);

    const std::string group1("group1");
    // NOTE: Duplicate property name differentiated by different group.
    props.AddProperty(group1, "prop1", 3);
    props.AddProperty(group1, "prop3", 4);
    props.AddProperty(group1, "prop4", 5);

    const std::string group2("group2");
    props.AddProperty(group2, "prop5", 6);
    return props;
  };

  // Only works for int-valued properties.
  auto properties_equal = [](
      const TestProperties& reference,
      const TestProperties& test) -> ::testing::AssertionResult {
    if (reference.num_groups() != test.num_groups()) {
      return ::testing::AssertionFailure()
          << "Different number of groups. Expected "
          << reference.num_groups() << " found " << test.num_groups();
    }

    for (const auto& group_name : reference.GetGroupNames()) {
      if (test.HasGroup(group_name)) {
        for (const auto& pair : reference.GetPropertiesInGroup(group_name)) {
          const std::string& name = pair.first;
          int expected_value = pair.second->get_value<int>();
          if (test.HasProperty(group_name, name)) {
            int test_value = test.GetProperty<int>(group_name, name);
            if (expected_value != test_value) {
              return ::testing::AssertionFailure()
                  << "Expected value for '" << group_name << "':'" << name
                  << "' to be " << expected_value << ". Found " << test_value;
            }
          } else {
            return ::testing::AssertionFailure()
                << "Expected group '" << group_name << "' to have property '"
                << name <<"'. It does not exist.";
          }
        }
      } else {
        return ::testing::AssertionFailure()
            << "Expected group '" << group_name
            << "' is missing from test properties";
      }
    }
    return ::testing::AssertionSuccess();
  };

  TestProperties source = make_properties();
  TestProperties reference = make_properties();

  // Copy construction.
  TestProperties copy_construct(source);
  EXPECT_TRUE(properties_equal(reference, copy_construct));

  // Copy assignment.
  TestProperties copy_assign;
  EXPECT_FALSE(properties_equal(reference, copy_assign));
  copy_assign = source;
  EXPECT_TRUE(properties_equal(reference, copy_assign));

  // Strictly speaking, confirming that the move *source* has changed isn't
  // necessary. The move semantics aren't documented. However, given that this
  // is using default move semantics on unordered_map, we can assume that the
  // source is modified by the move. So, we'll go ahead and test that.

  // Move construction.
  TestProperties move_construct(std::move(source));
  EXPECT_FALSE(properties_equal(reference, source));
  EXPECT_TRUE(properties_equal(reference, move_construct));

  // Move assignment.
  TestProperties move_assign;
  EXPECT_FALSE(properties_equal(reference, move_assign));
  move_assign = std::move(move_construct);
  EXPECT_FALSE(properties_equal(reference, move_construct));
  EXPECT_TRUE(properties_equal(reference, move_assign));
}

// Counts the number of instances constructed. Ignores destruction.
// TODO(eric.cousineau): Hoist this to more general testing code (e.g. for
// value_test.cc).
class GloballyCounted {
 public:
  struct Stats {
    int num_copies{};
    int num_moves{};

    ::testing::AssertionResult Equal(Stats other) {
      if (num_copies != other.num_copies || num_moves != other.num_moves) {
        return ::testing::AssertionFailure() <<
            fmt::format(
                "(num_copies, num_moves): ({}, {}) != ({}, {})",
                num_copies, num_moves, other.num_copies, other.num_moves);
      }
      return ::testing::AssertionSuccess();
    }
  };

  static Stats get_stats_and_reset() {
    Stats out = stats;
    stats = {0, 0};
    return out;
  }

  GloballyCounted() {}

  GloballyCounted(GloballyCounted&&) { stats.num_moves++; }
  GloballyCounted& operator=(GloballyCounted&&) {
    stats.num_moves++;
    return *this;
  }

  GloballyCounted(const GloballyCounted&) { stats.num_copies++; }
  GloballyCounted& operator=(const GloballyCounted&) {
    stats.num_copies++;
    return *this;
  }

 private:
  static Stats stats;
};

GloballyCounted::Stats GloballyCounted::stats;

GTEST_TEST(GeometryProperties, GloballyCounted) {
  // Unittest basic utility.
  const GloballyCounted value;
  EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({0, 0}));

  // Copy construction.
  {
    const GloballyCounted copy = value;
    unused(copy);
    EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({1, 0}));
  }

  // Copy assignment.
  {
    GloballyCounted copy;
    copy = value;
    EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({1, 0}));
  }

  // Move construction.
  {
    GloballyCounted moved_from;
    GloballyCounted moved_to = std::move(moved_from);
    unused(moved_to);
    EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({0, 1}));
  }

  // Move assigment.
  {
    GloballyCounted moved_from;
    GloballyCounted moved_to;
    moved_to = std::move(moved_from);
    EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({0, 1}));
  }
}

// Confirms the amount of copying that occurs.
GTEST_TEST(GeometryProperties, CopyCountCheck) {
  TestProperties properties;
  const std::string& group_name{"some_group"};
  const std::string name_1("name_1");
  const std::string name_2("name_2");

  // When adding a property, 2 copies should occur: once when constructing a
  // value, then another when cloning it.
  const GloballyCounted value;
  properties.AddPropertyAbstract(group_name, name_1, Value(value));
  EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({2, 0}));

  // Same as above.
  properties.AddProperty(group_name, name_2, value);
  EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({2, 0}));

  // No copies upon retrieving the type.
  properties.GetProperty<GloballyCounted>(group_name, name_1);
  EXPECT_TRUE(GloballyCounted::get_stats_and_reset().Equal({0, 0}));
}

GTEST_TEST(GeometryProperties, RgbaAndVector4) {
  const Rgba color(0.75, 0.5, 0.25, 1.);
  const Vector4d vector(0.75, 0.5, 0.25, 1.);

  TestProperties properties;
  const std::string& group_name{"some_group"};
  const std::string color_name("color_name");
  const std::string fake_name("fake_name");

  // Add<Rgba>.
  properties.AddProperty(group_name, color_name, color);
  // - Get<Rgba>.
  EXPECT_EQ(color, properties.GetProperty<Rgba>(group_name, color_name));
  // - Get<Vector4d>.
  EXPECT_EQ(vector, properties.GetProperty<Vector4d>(group_name, color_name));
  EXPECT_EQ(
      vector,
      properties.GetPropertyOrDefault<Vector4d>(
          group_name, fake_name, vector));

  // Add<Vector4d>.
  const std::string vector_name("vector_name");
  properties.AddProperty(group_name, vector_name, vector);
  // - Get<Rgba>.
  EXPECT_EQ(color, properties.GetProperty<Rgba>(group_name, vector_name));
  // - Get<Vector4d>.
  EXPECT_EQ(vector, properties.GetProperty<Vector4d>(group_name, vector_name));
}

}  // namespace
}  // namespace geometry
}  // namespace drake
