#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <string>
#include <memory>
#include <vector>
#include <fstream>

constexpr size_t MAX_MESSAGE_LENGTH = 20;

struct RingBufferHeader {
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
};

class FileManager {
public:
    explicit FileManager(const std::string& path, bool create_new = false);
    void write(const void* data, size_t size, std::streampos pos);
    void read(void* data, size_t size, std::streampos pos);
private:
    std::unique_ptr<std::fstream> file_;
};

class RingBuffer {
public:
    RingBuffer(const std::string& filename, size_t capacity, bool initialize = false);
    RingBuffer(const std::string& filename);
    
    void pushMessage(const std::string& message);
    std::string popMessage();
    bool isEmpty();
    bool isFull();

private:
    std::string filename_;
    size_t capacity_;
    std::unique_ptr<FileManager> file_manager_;
    
    void initializeBuffer();
    static const std::streampos HEADER_OFFSET;
};

#endif // RINGBUFFER_H
