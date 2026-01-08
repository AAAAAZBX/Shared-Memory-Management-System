#pragma once
#include <string>
#include <vector>

class SharedMemoryPool;

struct CommandSpec {
    std::string name;                    // e.g. "help"
    std::string summary;                 // one-liner
    std::string usage;                   // usage string
    std::vector<std::string> examples;   // examples
};


void HandleCommand(const std::vector<std::string>& tokens, SharedMemoryPool& smp);
void PrintHelpOverview();
void PrintHelpCommand(const std::string& cmd);

