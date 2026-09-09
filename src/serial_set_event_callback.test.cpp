#include <cpp_core/interface/serial_set_event_callback.h>

#include <cstdlib>

#include <gtest/gtest.h>

namespace
{
void portEvent(cpp_core::PortEvent, const char *)
{
}
} // namespace

TEST(SerialSetEventCallbackTest, EventCallbackCanBeStartedReplacedAndStoppedRepeatedly)
{
    EXPECT_EQ(serialSetEventCallback(nullptr), 0);
    for (int iteration = 0; iteration < 5; ++iteration)
    {
        EXPECT_EQ(serialSetEventCallback(portEvent), 0);
        EXPECT_EQ(serialSetEventCallback(portEvent), 0);
        EXPECT_EQ(serialSetEventCallback(nullptr), 0);
    }
    EXPECT_EQ(serialSetEventCallback(nullptr), 0);
}

TEST(SerialSetEventCallbackDeathTest, ActiveEventListenerIsStoppedOnExit)
{
    EXPECT_EXIT(
        {
            if (serialSetEventCallback(portEvent) != 0)
            {
                std::exit(1);
            }
            std::exit(0);
        },
        ::testing::ExitedWithCode(0), "");
}
