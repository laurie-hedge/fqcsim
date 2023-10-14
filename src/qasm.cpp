#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <string_view>

#include "constants.h"
#include "qasm.h"

struct Expression
{
	uint32_t line_number;
	std::vector<std::string_view> parts;
};

static void trim(std::string_view &string, char const *delims)
{
	string.remove_prefix(std::min(string.find_first_not_of(delims), string.size()));
	string.remove_suffix(std::min(string.size() - string.find_last_not_of(delims) - 1, string.size()));
}

static std::vector<std::string_view> split(std::string_view string, char const *delims)
{
	std::vector<std::string_view> parts;
	size_t pos_start = 0;
	trim(string, delims);
	while (pos_start < string.size()) {
		size_t const pos_end = string.find_first_of(delims, pos_start);
		if (pos_end == std::string_view::npos) {
			parts.push_back(string.substr(pos_start));
			pos_start = pos_end;
		} else {
			parts.push_back(string.substr(pos_start, pos_end - pos_start));
			pos_start = string.find_first_not_of(delims, pos_end + 1);
		}
	}
	return parts;
}

static std::optional<uint8_t> decode_operand(std::string_view operand)
{
	if (operand.size() < 2 || operand[0] != 'q') {
		return std::optional<uint8_t>();
	}
	size_t qbit_index = 0;
	for (size_t i = 1; i < operand.size(); ++i) {
		if (!std::isdigit(operand[i])) {
			return std::optional<uint8_t>();
		}
		qbit_index = (qbit_index * 10) + (operand[i] - '0');
	}
	if (qbit_index >= NUM_QBITS) {
		return std::optional<uint8_t>();
	}
	return std::optional<uint8_t>((uint8_t)qbit_index);
}

static std::optional<double> decode_immediate(std::string_view immediate)
{
	double result;
	auto [ptr, ec] = std::from_chars(immediate.data(), immediate.data() + immediate.size(), result);
	if (ec != std::errc() || ptr != (immediate.data() + immediate.size())) {
		return std::optional<double>();
	}
	return std::optional<double>(result);
}

Quantum_Program::Quantum_Program(std::string source_code)
{
	std::vector<Expression> expressions;

	std::transform(source_code.begin(), source_code.end(), source_code.begin(), [](char c) { return std::tolower(c); });

	uint32_t line_number = 1;
	std::vector<std::string_view> lines = split(source_code, "\n\r");
	for (auto &line : lines) {
		size_t const comment_start = line.find('#');
		if (comment_start != std::string_view::npos) {
			line = line.substr(0, comment_start);
		}

		std::vector<std::string_view> parts = split(line, " \t\v");
		parts.erase(std::remove_if(parts.begin(), parts.end(), [](std::string_view part) { return part.empty(); } ), parts.end());
		if (!parts.empty()) {
			expressions.push_back({line_number, parts});
		}

		line_number += 1;
	}

	for (auto const &expression : expressions) {
		if (expression.parts[0] == "cnot") {
			add_operation(Gate::CNOT, expression, 2);
		} else if (expression.parts[0] == "i") {
			add_operation(Gate::IDENTITY, expression, 1);
		} else if (expression.parts[0] == "h") {
			add_operation(Gate::HADAMARD, expression, 1);
		} else if (expression.parts[0] == "rx") {
			add_operation(Gate::R_X, expression, 1, true);
		} else if (expression.parts[0] == "ry") {
			add_operation(Gate::R_Y, expression, 1, true);
		} else if (expression.parts[0] == "rz") {
			add_operation(Gate::R_Z, expression, 1, true);
		} else if (expression.parts[0] == "s") {
			add_operation(Gate::S, expression, 1);
		} else if (expression.parts[0] == "sdag") {
			add_operation(Gate::S_DAG, expression, 1);
		} else if (expression.parts[0] == "swap") {
			add_operation(Gate::SWAP, expression, 2);
		} else if (expression.parts[0] == "t") {
			add_operation(Gate::T, expression, 1);
		} else if (expression.parts[0] == "tdag") {
			add_operation(Gate::T_DAG, expression, 1);
		} else if (expression.parts[0] == "toffoli") {
			add_operation(Gate::TOFFOLI, expression, 3);
		} else if (expression.parts[0] == "x") {
			add_operation(Gate::PAULI_X, expression, 1);
		} else if (expression.parts[0] == "y") {
			add_operation(Gate::PAULI_Y, expression, 1);
		} else if (expression.parts[0] == "z") {
			add_operation(Gate::PAULI_Z, expression, 1);
		} else {
			set_error(expression.line_number, "Unknown gate '" + std::string(expression.parts[0]) + "'");
		}

		if (!valid) {
			return;
		}
	}

	std::sort(active_qbits.begin(), active_qbits.end(), std::greater<>());
}

bool Quantum_Program::is_valid() const
{
	return valid;
}

std::string Quantum_Program::get_build_error() const
{
	return error_message;
}

std::vector<Operation> const &Quantum_Program::get_operations() const
{
	return operations;
}

std::vector<uint8_t> const &Quantum_Program::get_active_qbits() const
{
	return active_qbits;
}

void Quantum_Program::add_operation(Gate gate, Expression const &expression, uint8_t num_operands, bool has_immediate)
{
	if (expression.parts.size() != (num_operands + has_immediate + 1)) {
		set_error(expression.line_number,
				  "Gate " + std::string(expression.parts[0]) + " expects " + std::to_string(num_operands) + " operands, " +
				  std::to_string(expression.parts.size() - 1) + " operands found");
		return;
	}

	Operation operation = { gate };
	for (uint8_t index = 0; index < num_operands; ++index) {
		std::optional<uint8_t> operand = decode_operand(expression.parts[index + 1]);
		if (operand) {
			operation.operands[index] = *operand;
			if (std::find(active_qbits.begin(), active_qbits.end(), *operand) == active_qbits.end()) {
				active_qbits.push_back(*operand);
			}
		} else {
			set_error(expression.line_number, "Invalid operand " + std::string(expression.parts[index + 1]));
			return;
		}
	}

	for (uint8_t a = 0; a < num_operands; ++a) {
		for (uint8_t b = a + 1; b < num_operands; ++b) {
			if (operation.operands[a] == operation.operands[b]) {
				set_error(expression.line_number, "Operand " + std::to_string(a + 1) + " and operand " + std::to_string(b + 1) + " reference the same qbit; operands must be unique");
				return;
			}
		}
	}

	if (has_immediate) {
		std::optional<double> immediate = decode_immediate(expression.parts[num_operands + 1]);
		if (immediate) {
			operation.immediate = *immediate;
		} else {
			set_error(expression.line_number, "Invalid immediate " + std::string(expression.parts[num_operands + 1]));
			return;
		}
	}

	operations.push_back(operation);
}

void Quantum_Program::set_error(uint32_t line_number, std::string const &error)
{
	valid = false;
	error_message = "Error on line " + std::to_string(line_number) + ": " + error;
}
