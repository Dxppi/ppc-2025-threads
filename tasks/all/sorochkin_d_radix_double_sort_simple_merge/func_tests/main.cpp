#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

#include "../include/ops.hpp"
#include "boost/mpi/communicator.hpp"
#include "core/task/include/task.hpp"

namespace {
std::vector<double> MakeUnsortedInput(std::size_t size) {
  std::vector<double> v(size);
  std::iota(v.rbegin(), v.rend(), 0);
  return v;
}
}  // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
class sorochkin_d_radix_double_sort_simple_merge_test_all : public ::testing::Test {
 private:
  static std::vector<double> MakeRandomInput(std::size_t size) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dist(-64000, 64000);

    std::vector<double> v(size);
    std::ranges::generate(v, [&dist, &gen] { return dist(gen); });

    return v;
  }

 public:
  static void RunTest(std::vector<double> &&in) {
    boost::mpi::communicator world;

    std::vector<double> out(in.size());

    auto data = std::make_shared<ppc::core::TaskData>(world.rank() == 0 ? ppc::core::TaskData{
        .inputs = {reinterpret_cast<uint8_t *>(in.data())},
        .inputs_count = {static_cast<std::uint32_t>(in.size())},
        .outputs = {reinterpret_cast<uint8_t *>(out.data())},
        .outputs_count = {static_cast<unsigned int>(out.size())},
        .state_of_testing = {},
    } : ppc::core::TaskData{});

    auto task = sorochkin_d_radix_double_sort_simple_merge_all::SortTask(data);
    ASSERT_TRUE(task.Validation());
    task.PreProcessing();
    task.Run();
    task.PostProcessing();

    EXPECT_TRUE(std::ranges::is_sorted(out));
  }

  static void RunTest(std::size_t size) { RunTest(MakeRandomInput(size)); }

  static void RunInvalidSizeTest(int diff) {
    boost::mpi::communicator world;
    if (world.rank() != 0) {
      return;
    }

    std::vector<double> in(64);
    std::vector<double> out(in.size() + diff);

    auto data = std::make_shared<ppc::core::TaskData>(ppc::core::TaskData{
        .inputs = {reinterpret_cast<uint8_t *>(in.data())},
        .inputs_count = {static_cast<std::uint32_t>(in.size())},
        .outputs = {reinterpret_cast<uint8_t *>(out.data())},
        .outputs_count = {static_cast<unsigned int>(out.size())},
        .state_of_testing = {},
    });

    auto task = sorochkin_d_radix_double_sort_simple_merge_all::SortTask(data);
    EXPECT_FALSE(task.Validation());
  }
};

namespace {

TEST_F(sorochkin_d_radix_double_sort_simple_merge_test_all, invalid_more) { RunInvalidSizeTest(1); }
TEST_F(sorochkin_d_radix_double_sort_simple_merge_test_all, invalid_less) { RunInvalidSizeTest(-1); }

// NOLINTNEXTLINE(readability-identifier-naming)
class sized_random : public sorochkin_d_radix_double_sort_simple_merge_test_all,
                     public ::testing::WithParamInterface<std::size_t> {};

TEST_P(sized_random, random_test) { RunTest(GetParam()); }
// NOLINTNEXTLINE(misc-use-anonymous-namespace)
INSTANTIATE_TEST_SUITE_P(sorochkin_d_radix_double_sort_simple_merge_test_all, sized_random,
                         testing::Range(std::size_t(0), std::size_t(100)));

// NOLINTNEXTLINE(readability-identifier-naming)
class special : public sorochkin_d_radix_double_sort_simple_merge_test_all,
                public ::testing::WithParamInterface<std::vector<double>> {};

TEST_P(special, random_test) { RunTest(std::vector<double>(GetParam())); }
// NOLINTNEXTLINE(misc-use-anonymous-namespace)
INSTANTIATE_TEST_SUITE_P(sorochkin_d_radix_double_sort_simple_merge_test_all, special,
                         testing::Values(std::vector<double>(32, 0), std::vector<double>(64, 0),
                                         std::vector<double>(32, 1), std::vector<double>(64, 1),
                                         std::vector<double>(32, -1), std::vector<double>(64, -1), MakeUnsortedInput(2),
                                         MakeUnsortedInput(3), MakeUnsortedInput(4), MakeUnsortedInput(5),
                                         MakeUnsortedInput(7), MakeUnsortedInput(13), MakeUnsortedInput(15),
                                         MakeUnsortedInput(32), MakeUnsortedInput(48), MakeUnsortedInput(50),
                                         MakeUnsortedInput(64), MakeUnsortedInput(73), MakeUnsortedInput(100)));
}  // namespace
