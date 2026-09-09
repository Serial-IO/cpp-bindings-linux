#include <cpp_core/serial.h>
#include <cpp_core/status_code.h>

#include "detail/handle_types.hpp"

#include <cstdlib>
#include <fcntl.h>

#include <gtest/gtest.h>

TEST(SerialSetStopBitsTest, UpdatesTypedStopBitsAndRejectsInvalidValues)
{
    constexpr auto kConfig = cpp_core::SerialConfig::make<9600, cpp_core::DataBits::kEight>();
    using cpp_bindings_linux::detail::UniqueFd;
    UniqueFd master(posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC));
    ASSERT_TRUE(master.valid());
    ASSERT_EQ(grantpt(master.get()), 0);
    ASSERT_EQ(unlockpt(master.get()), 0);
    const char *path = ptsname(master.get());
    ASSERT_NE(path, nullptr);
    for (auto flow : {cpp_core::FlowControl::kNone, cpp_core::FlowControl::kRtsCts, cpp_core::FlowControl::kXonXoff})
    {
        auto config = kConfig;
        config.flow_mode = flow;
        const auto handle = serialOpen(path, &config);
        ASSERT_GT(handle, 0);
        UniqueFd slave(static_cast<int>(handle));
        EXPECT_EQ(serialSetStopBits(handle, cpp_core::StopBits::kTwo), 0);
        EXPECT_EQ(serialGetStopBits(handle), cpp_core::StopBits::kTwo);
        EXPECT_EQ(serialSetStopBits(handle, static_cast<cpp_core::StopBits>(1)),
                  static_cast<int>(cpp_core::StatusCode::Configuration::kSetStopBitsError));
        EXPECT_EQ(serialClose(slave.release()), 0);
    }
}
