#include "RingBuffer.h"
#include <stdexcept>
#include <cstring>

const std::streampos RingBuffer::HEADER_OFFSET = std::streampos(0);

FileManager::FileManager(const std::string& path, bool create_new) {
    auto mode = std::ios::in | std::ios::out | std::ios::binary;
    if (create_new) {
        mode |= std::ios::trunc;
    }
    
    file_ = std::make_unique<std::fstream>(path, mode);
    if (!file_->is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
}

void FileManager::write(const void* data, size_t size, std::streampos pos) {
    file_->seekp(pos);
    file_->write(static_cast<const char*>(data), size);
    if (!file_->good()) {
        throw std::runtime_error("Write operation failed");
    }
    file_->flush();
}

void FileManager::read(void* data, size_t size, std::streampos pos) {
    file_->seekg(pos);
    file_->read(static_cast<char*>(data), size);
    if (!file_->good()) {
        throw std::runtime_error("Read operation failed");
    }
}

RingBuffer::RingBuffer(const std::string& filename, size_t capacity, bool initialize)
    : filename_(filename), capacity_(capacity), file_manager_(std::make_unique<FileManager>(filename, initialize)) {
    if (initialize) {
        initializeBuffer();
    }
}

RingBuffer::RingBuffer(const std::string& filename)
    : filename_(filename), file_manager_(std::make_unique<FileManager>(filename)) {
    RingBufferHeader header;
    file_manager_->read(&header, sizeof(header), HEADER_OFFSET);
    capacity_ = header.capacity;
}

void RingBuffer::initializeBuffer() {
    RingBufferHeader header{};
    header.capacity = capacity_;
    header.head = header.tail = header.count = 0;
    
    file_manager_->write(&header, sizeof(header), HEADER_OFFSET);
    
    std::vector<char> empty(MAX_MESSAGE_LENGTH, 0);
    for (size_t i = 0; i < capacity_; ++i) {
        file_manager_->write(empty.data(), MAX_MESSAGE_LENGTH, 
                           sizeof(RingBufferHeader) + i * MAX_MESSAGE_LENGTH);
    }
}

void RingBuffer::pushMessage(const std::string& message) {
    if (message.length() > MAX_MESSAGE_LENGTH) {
        throw std::runtime_error("Message too long");
    }
    
    RingBufferHeader header;
    file_manager_->read(&header, sizeof(header), HEADER_OFFSET);
    
    if (header.count >= header.capacity) {
        throw std::runtime_error("Buffer full");
    }
    
    std::vector<char> buffer(MAX_MESSAGE_LENGTH, 0);
    std::memcpy(buffer.data(), message.c_str(), message.length());
    
    auto pos = sizeof(RingBufferHeader) + header.tail * MAX_MESSAGE_LENGTH;
    file_manager_->write(buffer.data(), MAX_MESSAGE_LENGTH, pos);
    
    header.tail = (header.tail + 1) % header.capacity;
    header.count++;
    file_manager_->write(&header, sizeof(header), HEADER_OFFSET);
}

std::string RingBuffer::popMessage() {
    RingBufferHeader header;
    file_manager_->read(&header, sizeof(header), HEADER_OFFSET);
    
    if (header.count == 0) {
        throw std::runtime_error("Buffer empty");
    }
    
    std::vector<char> buffer(MAX_MESSAGE_LENGTH);
    auto pos = sizeof(RingBufferHeader) + header.head * MAX_MESSAGE_LENGTH;
    file_manager_->read(buffer.data(), MAX_MESSAGE_LENGTH, pos);
    
    std::vector<char> empty(MAX_MESSAGE_LENGTH, 0);
    file_manager_->write(empty.data(), MAX_MESSAGE_LENGTH, pos);
    
    header.head = (header.head + 1) % header.capacity;
    header.count--;
    file_manager_->write(&header, sizeof(header), HEADER_OFFSET);
    
    return std::string(buffer.data());
}

bool RingBuffer::isEmpty() {
    RingBufferHeader header;
    file_manager_->read(&header, sizeof(header), HEADER_OFFSET);
    return header.count == 0;
}

bool RingBuffer::isFull() {
    RingBufferHeader header;
    file_manager_->read(&header, sizeof(header), HEADER_OFFSET);
    return header.count == header.capacity;
}
