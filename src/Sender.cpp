#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include "RingBuffer.h"

class MessageSender {
public:
    explicit MessageSender(const std::string& filename) 
        : buffer_(std::make_unique<RingBuffer>(filename)) {}
    
    void run() {
        std::cout << "Sender is ready. Enter command (send/exit):" << std::endl;
        
        while (true) {
            std::cout << "Sender command: ";
            std::string cmd;
            std::getline(std::cin, cmd);
            
            if (cmd == "send") {
                sendMessage();
            } else if (cmd == "exit") {
                break;
            } else {
                std::cout << "Unknown command." << std::endl;
            }
        }
    }
    
private:
    std::unique_ptr<RingBuffer> buffer_;
    
    void sendMessage() {
        std::cout << "Enter message (max 20 chars): ";
        std::string msg;
        std::getline(std::cin, msg);
        
        try {
            buffer_->pushMessage(msg);
            std::cout << "Message sent." << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Send error: " << e.what() << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: Sender <filename>" << std::endl;
            return 1;
        }
        
        MessageSender sender(argv[1]);
        sender.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Sender error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
