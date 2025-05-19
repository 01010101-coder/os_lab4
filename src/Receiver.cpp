#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <cstdlib>
#include "RingBuffer.h"

class MessageReceiver {
public:
    void run() {
        setupBuffer();
        launchSenders();
        processMessages();
    }
    
private:
    std::unique_ptr<RingBuffer> buffer_;
    std::string filename_;
    
    void setupBuffer() {
        std::cout << "Enter binary file name: ";
        std::getline(std::cin, filename_);
        
        size_t capacity;
        std::cout << "Enter number of records (capacity): ";
        std::cin >> capacity;
        std::cin.ignore();
        
        buffer_ = std::make_unique<RingBuffer>(filename_, capacity, true);
    }
    
    void launchSenders() {
        int num_senders;
        std::cout << "Enter number of Sender processes to launch: ";
        std::cin >> num_senders;
        std::cin.ignore();
        
        for (int i = 0; i < num_senders; ++i) {
            std::string cmd = getSenderCommand();
            std::cout << "Launching Sender process " << i + 1 << std::endl;
            
            if (system(cmd.c_str()) != 0) {
                std::cerr << "Failed to launch Sender process " << i + 1 << std::endl;
            }
        }
        
        std::cout << "Press Enter when all Sender processes are ready...";
        std::cin.get();
    }
    
    std::string getSenderCommand() const {
        #ifdef _WIN32
            return "start Sender " + filename_;
        #else
            return "./Sender " + filename_;
        #endif
    }
    
    void processMessages() {
        while (true) {
            std::cout << "Enter command (read/exit): ";
            std::string cmd;
            std::getline(std::cin, cmd);
            
            if (cmd == "read") {
                readMessage();
            } else if (cmd == "exit") {
                break;
            } else {
                std::cout << "Unknown command." << std::endl;
            }
        }
    }
    
    void readMessage() {
        try {
            std::string msg = buffer_->popMessage();
            std::cout << "Received message: " << msg << std::endl;
        } catch (const std::exception& e) {
            std::cout << "No messages available: " << e.what() << std::endl;
        }
    }
};

int main() {
    try {
        MessageReceiver receiver;
        receiver.run();
    } catch (const std::exception& e) {
        std::cerr << "Receiver error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
