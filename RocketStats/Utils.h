#pragma once

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <functional>

using namespace std;


class Utils
{
public:
	static void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
	static void ReplaceVars(std::string& str, std::map<std::string, std::string>& vars, std::function<void(const std::string&, std::string&)> passe = nullptr);
	static std::string FloatFixer(float a_value, size_t n);
	static std::vector<std::string> Split(const std::string& str, char delim);
	static std::map<std::string, int> SplitKeyInt(const std::string str, size_t offset = 0);
	static size_t FindKeyInt(std::vector<std::map<std::string, int>> vector, std::string key, int value);

	// Colors
	static unsigned char OpacityColor(float opacity);
	static unsigned char GetAlpha(std::vector<float> color, float opacity);

	// Operations
	static std::string ExpressionSanitize(std::string str, int percent2pixels);
	static int EvaluateExpression(std::string exp);
	static void ProcessClosingParenthesis(std::vector<int>& vStack, std::vector<char>& opStack);
	static int ProcessInputNumber(std::string& exp, int pos, std::vector<int>& vStack);
	static void ProcessInputOperator(char op, std::vector<int>& vStack, std::vector<char>& opStack);
	static bool OperatorCausesEvaluation(char op, char prevOp);
	static void ExecuteOperation(std::vector<int>& vStack, std::vector<char>& opStack);
};