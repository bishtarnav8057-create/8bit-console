#include "OperandCheck.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>

namespace {
struct Spec { int count; const char* first; const char* second; };
// Operand types: R=register, V=8-bit immediate value, A=16-bit address/label.
const std::unordered_map<std::string, Spec> ISA = {
    {"MOV", {2,"R","R_OR_V"}}, {"LOAD", {2,"R","V_OR_A"}},
    {"STORE",{2,"A_OR_R","R_OR_V"}}, {"ADD",{2,"R","R_OR_V"}},
    {"SUB",{2,"R","R_OR_V"}}, {"AND",{2,"R","R_OR_V"}},
    {"OR", {2,"R","R_OR_V"}}, {"XOR",{2,"R","R_OR_V"}},
    {"CMP",{2,"R","R_OR_V"}}, {"JMP",{1,"A",nullptr}},
    {"JZ", {1,"A",nullptr}}, {"JNZ",{1,"A",nullptr}},
    {"PUSH",{1,"R",nullptr}}, {"POP",{1,"R",nullptr}},
    {"CALL",{1,"A",nullptr}}, {"RET",{0,nullptr,nullptr}},
    {"HLT",{0,nullptr,nullptr}}, {"NOP",{0,nullptr,nullptr}}
};
std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return s;
}
bool isRegister(const std::string& raw) {
    std::string s=upper(raw);
    if (s.size()==1 && s[0]>='A' && s[0]<='D') return true;
    return s.size()==2 && s[0]=='R' && s[1]>='0' && s[1]<='7';
}
bool parseUnsigned(const std::string& s, unsigned long& value) {
    if(s.empty()) return false;
    char* end=nullptr; value=std::strtoul(s.c_str(), &end, 0);
    return end==s.c_str()+s.size() && end!=s.c_str() && s[0]!='-';
}
bool isLabelOrAddress(const std::string& s) {
    unsigned long n=0;
    if(parseUnsigned(s,n)) return n<=65535;
    if(s.empty() || !(std::isalpha(static_cast<unsigned char>(s[0])) || s[0]=='_')) return false;
    return std::all_of(s.begin()+1,s.end(),[](unsigned char c){return std::isalnum(c)||c=='_';});
}
bool isValue(const std::string& s) { unsigned long n=0; return parseUnsigned(s,n) && n<=255; }
bool typeMatches(const std::string& type,const std::string& op) {
    if(type.empty()) return false;
    if(type=="R") return isRegister(op);
    if(type=="V") return isValue(op);
    if(type=="A") return isLabelOrAddress(op);
    if(type=="R_OR_V") return isRegister(op)||isValue(op);
    if(type=="V_OR_A") return isValue(op)||isLabelOrAddress(op);
    if(type=="A_OR_R") return isLabelOrAddress(op)||isRegister(op);
    return false;
}
}

OperandCheckResult checkInstruction(const Instruction& inst) {
    const std::string name=upper(inst.name);
    auto it=ISA.find(name);
    if(it==ISA.end()) return {false,"Unknown instruction '"+inst.name+"'"};
    if(static_cast<int>(inst.operands.size())!=it->second.count)
        return {false,name+" expects "+std::to_string(it->second.count)+" operand(s), got "+std::to_string(inst.operands.size())};
    if(it->second.count>=1 && !typeMatches(it->second.first,inst.operands[0]))
        return {false,"Invalid first operand '"+inst.operands[0]+"' for "+name};
    if(it->second.count>=2 && !typeMatches(it->second.second,inst.operands[1]))
        return {false,"Invalid second operand '"+inst.operands[1]+"' for "+name};
    return {true,"OK"};
}

std::vector<OperandCheckResult> checkProgram(const std::vector<Instruction>& instructions) {
    std::vector<OperandCheckResult> results;
    results.reserve(instructions.size());
    for(const auto& inst: instructions) results.push_back(checkInstruction(inst));
    return results;
}
