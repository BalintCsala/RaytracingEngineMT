#ifndef DIPTERV_RT_EXPRESSION_PARSING_H
#define DIPTERV_RT_EXPRESSION_PARSING_H

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

enum class Operator {
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
};

inline bool isNumber(char c) {
    return c >= '0' && c <= '9';
}

inline bool isVariable(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline int readNumber(const std::string &line, size_t &index) {
    auto startIndex = index;
    auto endIndex = startIndex + 1;
    while (isNumber(line[endIndex]) && endIndex < line.size()) {
        endIndex++;
    }
    index = endIndex;
    return std::stoi(line.substr(startIndex, endIndex));
}

inline std::string readVariable(const std::string &line, size_t &index) {
    auto startIndex = index;
    auto endIndex = startIndex + 1;
    while (isVariable(line[endIndex]) && endIndex < line.size()) {
        endIndex++;
    }
    index = endIndex;
    return line.substr(startIndex, endIndex);
}

inline int32_t parseExpression(const std::string &expression, const std::unordered_map<std::string, int32_t> &variables) {
    std::vector<Operator> operatorStack;
    std::vector<int32_t> outputStack;
    size_t index = 0;
    while (index < expression.size()) {
        char c = expression[index];
        if (c == ' ') {
            index++;
            continue;
        }
        
        if (isNumber(c)) {
            outputStack.push_back(readNumber(expression, index));
        } else if (isVariable(c)) {
            outputStack.push_back(variables.at(readVariable(expression, index)));
        } else {
            switch (c) {
                case '+':
                    operatorStack.push_back(Operator::ADD);
                    break;
                case '-':
                    operatorStack.push_back(Operator::SUBTRACT);
                    break;
                case '*':
                    operatorStack.push_back(Operator::MULTIPLY);
                    break;
                case '/':
                    operatorStack.push_back(Operator::DIVIDE);
                    break;
                default:
                    throw std::runtime_error("Failed to parse expression: " + expression);
            }
            index++;
        }
    }

    while (!operatorStack.empty()) {
        auto op = operatorStack.back();
        operatorStack.pop_back();
        auto b = outputStack.back();
        outputStack.pop_back();
        auto a = outputStack.back();
        outputStack.pop_back();
        switch (op) {
            case Operator::ADD:
                outputStack.push_back(a + b);
                break;
            case Operator::SUBTRACT:
                outputStack.push_back(a - b);
                break;
            case Operator::MULTIPLY:
                outputStack.push_back(a * b);
                break;
            case Operator::DIVIDE:
                outputStack.push_back(a / b);
                break;
        }
    }
    if (outputStack.size() != 1) {
        throw std::runtime_error("Failed to parse expression (too many values remain): " + expression);
    }
    return outputStack.back();
}

#endif //DIPTERV_RT_EXPRESSION_PARSING_H
