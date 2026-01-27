#include <iostream>
#include "CommandRegistry.h"

int main() {
    CommandRegistry registry;

    registry.registerCommand("open_notepad", []() {
        std::cout << "Notepad command executed (mock)\n";
        });

    registry.executeCommand("open_notepad");
    registry.executeCommand("format_c_drive"); // should be blocked

    return 0;
}
