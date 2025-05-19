#include "gtest/gtest.h"
#include "RingBuffer.h"
#include <cstdio>
#include <thread>
#include <vector>
#include <chrono>

// Test: Basic push/pop
TEST(RingBufferTest, PushPopSimple) {
    const std::string testFile = "test_ringbuffer_simple.bin";
    size_t capacity = 3;
    {
        RingBuffer rb(testFile, capacity, true);
        EXPECT_TRUE(rb.isEmpty());
        EXPECT_FALSE(rb.isFull());
        rb.pushMessage("Hello");
        EXPECT_FALSE(rb.isEmpty());
        std::string msg = rb.popMessage();
        EXPECT_EQ(msg, "Hello");
        EXPECT_TRUE(rb.isEmpty());
    }
    std::remove(testFile.c_str());
}

// Test: Full buffer
TEST(RingBufferTest, FullBuffer) {
    const std::string testFile = "test_ringbuffer_full.bin";
    size_t capacity = 2;
    {
        RingBuffer rb(testFile, capacity, true);
        rb.pushMessage("First");
        rb.pushMessage("Second");
        EXPECT_TRUE(rb.isFull());
        EXPECT_THROW(rb.pushMessage("Third"), std::runtime_error);
    }
    std::remove(testFile.c_str());
}

// Test: Empty buffer
TEST(RingBufferTest, EmptyBuffer) {
    const std::string testFile = "test_ringbuffer_empty.bin";
    size_t capacity = 2;
    {
        RingBuffer rb(testFile, capacity, true);
        EXPECT_THROW(rb.popMessage(), std::runtime_error);
    }
    std::remove(testFile.c_str());
}

// Test: Wrap-around behavior
TEST(RingBufferTest, WrapAroundBehavior) {
    const std::string testFile = "test_ringbuffer_wrap.bin";
    size_t capacity = 3;
    {
        RingBuffer rb(testFile, capacity, true);
        rb.pushMessage("Msg1");
        rb.pushMessage("Msg2");
        rb.pushMessage("Msg3");
        EXPECT_TRUE(rb.isFull());
        EXPECT_EQ(rb.popMessage(), "Msg1");
        rb.pushMessage("Msg4");
        EXPECT_TRUE(rb.isFull());
        EXPECT_EQ(rb.popMessage(), "Msg2");
        EXPECT_EQ(rb.popMessage(), "Msg3");
        EXPECT_EQ(rb.popMessage(), "Msg4");
        EXPECT_TRUE(rb.isEmpty());
    }
    std::remove(testFile.c_str());
}

// Test: Message length exceeded
TEST(RingBufferTest, MessageLengthExceeded) {
    const std::string testFile = "test_ringbuffer_length.bin";
    size_t capacity = 2;
    {
        RingBuffer rb(testFile, capacity, true);
        std::string longMsg(MAX_MESSAGE_LENGTH + 1, 'a');
        EXPECT_THROW(rb.pushMessage(longMsg), std::runtime_error);
    }
    std::remove(testFile.c_str());
}

// Test: Persistence across instances
TEST(RingBufferTest, PersistenceAcrossInstances) {
    const std::string testFile = "test_ringbuffer_persistence.bin";
    size_t capacity = 3;
    {
        {
            RingBuffer rb(testFile, capacity, true);
            rb.pushMessage("One");
            rb.pushMessage("Two");
        }
        {
            RingBuffer rb(testFile);
            EXPECT_EQ(rb.popMessage(), "One");
            EXPECT_EQ(rb.popMessage(), "Two");
        }
    }
    std::remove(testFile.c_str());
}

// Test: Multiple messages with different lengths
TEST(RingBufferTest, MultipleMessageLengths) {
    const std::string testFile = "test_ringbuffer_multiple_lengths.bin";
    size_t capacity = 5;
    {
        RingBuffer rb(testFile, capacity, true);
        rb.pushMessage("A");  // 1 char
        rb.pushMessage("Hello");  // 5 chars
        rb.pushMessage("Hello World!");  // 12 chars
        rb.pushMessage("");  // empty string
        rb.pushMessage(std::string(MAX_MESSAGE_LENGTH - 1, 'x'));  // max length - 1
        
        EXPECT_EQ(rb.popMessage(), "A");
        EXPECT_EQ(rb.popMessage(), "Hello");
        EXPECT_EQ(rb.popMessage(), "Hello World!");
        EXPECT_EQ(rb.popMessage(), "");
        EXPECT_EQ(rb.popMessage(), std::string(MAX_MESSAGE_LENGTH - 1, 'x'));
    }
    std::remove(testFile.c_str());
}

// Test: Concurrent access from multiple threads
TEST(RingBufferTest, ConcurrentAccess) {
    const std::string testFile = "test_ringbuffer_concurrent.bin";
    size_t capacity = 10;
    {
        RingBuffer rb(testFile, capacity, true);
        std::vector<std::thread> threads;
        std::vector<std::string> results;
        std::mutex results_mutex;
        
        // Create 5 writer threads
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&rb, i]() {
                try {
                    rb.pushMessage("Thread" + std::to_string(i));
                } catch (const std::exception&) {
                    // Ignore full buffer exceptions
                }
            });
        }
        
        // Create 5 reader threads
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&rb, &results, &results_mutex]() {
                try {
                    std::string msg = rb.popMessage();
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results.push_back(msg);
                } catch (const std::exception&) {
                    // Ignore empty buffer exceptions
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        EXPECT_LE(results.size(), 5);  // Should have at most 5 messages
    }
    std::remove(testFile.c_str());
}

// Test: Buffer state after multiple operations
TEST(RingBufferTest, BufferStateAfterOperations) {
    const std::string testFile = "test_ringbuffer_state.bin";
    size_t capacity = 3;
    {
        RingBuffer rb(testFile, capacity, true);
        EXPECT_TRUE(rb.isEmpty());
        EXPECT_FALSE(rb.isFull());
        
        rb.pushMessage("First");
        EXPECT_FALSE(rb.isEmpty());
        EXPECT_FALSE(rb.isFull());
        
        rb.pushMessage("Second");
        EXPECT_FALSE(rb.isEmpty());
        EXPECT_FALSE(rb.isFull());
        
        rb.pushMessage("Third");
        EXPECT_FALSE(rb.isEmpty());
        EXPECT_TRUE(rb.isFull());
        
        rb.popMessage();
        EXPECT_FALSE(rb.isEmpty());
        EXPECT_FALSE(rb.isFull());
        
        rb.popMessage();
        EXPECT_FALSE(rb.isEmpty());
        EXPECT_FALSE(rb.isFull());
        
        rb.popMessage();
        EXPECT_TRUE(rb.isEmpty());
        EXPECT_FALSE(rb.isFull());
    }
    std::remove(testFile.c_str());
}

// Test: File operations error handling
TEST(RingBufferTest, FileOperationsErrorHandling) {
    const std::string testFile = "/invalid/path/test.bin";
    EXPECT_THROW(RingBuffer rb(testFile, 5, true), std::runtime_error);
}

// Test: Message content preservation
TEST(RingBufferTest, MessageContentPreservation) {
    const std::string testFile = "test_ringbuffer_content.bin";
    size_t capacity = 2;
    {
        RingBuffer rb(testFile, capacity, true);
        // Используем короткие строки со специальными символами
        std::string specialChars = "!@#$%";
        rb.pushMessage(specialChars);
        EXPECT_EQ(rb.popMessage(), specialChars);
        
        std::string asciiChars = "Hello";
        rb.pushMessage(asciiChars);
        EXPECT_EQ(rb.popMessage(), asciiChars);
    }
    std::remove(testFile.c_str());
}

// Test: Buffer capacity limits
TEST(RingBufferTest, BufferCapacityLimits) {
    const std::string testFile = "test_ringbuffer_capacity.bin";
    {
        // Test with minimum capacity
        RingBuffer rb(testFile, 1, true);
        rb.pushMessage("Test");
        EXPECT_THROW(rb.pushMessage("Another"), std::runtime_error);
        
        // Test with large capacity
        std::remove(testFile.c_str());
        RingBuffer rb2(testFile, 1000, true);
        for (int i = 0; i < 1000; ++i) {
            rb2.pushMessage("Message" + std::to_string(i));
        }
        EXPECT_TRUE(rb2.isFull());
        EXPECT_THROW(rb2.pushMessage("Extra"), std::runtime_error);
    }
    std::remove(testFile.c_str());
}

// Test: Message ordering
TEST(RingBufferTest, MessageOrdering) {
    const std::string testFile = "test_ringbuffer_ordering.bin";
    size_t capacity = 5;
    {
        RingBuffer rb(testFile, capacity, true);
        std::vector<std::string> messages = {"First", "Second", "Third", "Fourth", "Fifth"};
        
        for (const auto& msg : messages) {
            rb.pushMessage(msg);
        }
        
        for (const auto& msg : messages) {
            EXPECT_EQ(rb.popMessage(), msg);
        }
    }
    std::remove(testFile.c_str());
}

// Test: Buffer reuse
TEST(RingBufferTest, BufferReuse) {
    const std::string testFile = "test_ringbuffer_reuse.bin";
    size_t capacity = 3;
    {
        RingBuffer rb(testFile, capacity, true);
        rb.pushMessage("First");
        rb.pushMessage("Second");
        rb.pushMessage("Third");
        
        EXPECT_EQ(rb.popMessage(), "First");
        rb.pushMessage("Fourth");
        
        EXPECT_EQ(rb.popMessage(), "Second");
        EXPECT_EQ(rb.popMessage(), "Third");
        EXPECT_EQ(rb.popMessage(), "Fourth");
    }
    std::remove(testFile.c_str());
}

// Test: Stress test
TEST(RingBufferTest, StressTest) {
    const std::string testFile = "test_ringbuffer_stress.bin";
    size_t capacity = 100;
    {
        RingBuffer rb(testFile, capacity, true);
        for (int i = 0; i < 1000; ++i) {
            rb.pushMessage("Message" + std::to_string(i));
            rb.popMessage();
        }
        EXPECT_TRUE(rb.isEmpty());
    }
    std::remove(testFile.c_str());
}

// Test: Invalid operations
TEST(RingBufferTest, InvalidOperations) {
    const std::string testFile = "test_ringbuffer_invalid.bin";
    size_t capacity = 2;
    {
        RingBuffer rb(testFile, capacity, true);
        EXPECT_THROW(rb.pushMessage(std::string(MAX_MESSAGE_LENGTH + 1, 'x')), std::runtime_error);
        EXPECT_THROW(rb.popMessage(), std::runtime_error);
        
        rb.pushMessage("Test");
        rb.popMessage();
        EXPECT_THROW(rb.popMessage(), std::runtime_error);
    }
    std::remove(testFile.c_str());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
