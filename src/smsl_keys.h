#include <unordered_map>
#include <string>
#include <utility> // For std::pair

std::unordered_map<std::string, std::pair<uint16_t, uint16_t>> IR_CODES = {
    {"POWER",   { 0x3412, 0x1}},
    {"UP",      { 0x3412, 0x2}},
    {"LEFT",    { 0x3412, 0x3}},
    {"ENTER",   { 0x3412, 0x4}},
    {"RIGHT",   { 0x3412, 0x5}},
    {"DOWN",    { 0x3412, 0x6}},
    {"SOURCE",  { 0x3412, 0x7}},
    {"FN",      { 0x3412, 0x8}},
    {"MUTE",    { 0x3412, 0x9}}
};
