#pragma once

#include <array>
#include <string>
#include <vector>

enum class Gate : uint8_t
{
	CNOT,
	IDENTITY,
	HADAMARD,
	PAULI_X,
	PAULI_Y,
	PAULI_Z,
	R_X,
	R_Y,
	R_Z,
	S,
	S_DAG,
	SWAP,
	T,
	T_DAG,
	TOFFOLI,
};

struct Operation
{
	Gate gate;
	std::array<uint8_t, 3> operands;
	double immediate;
};

class Quantum_Program
{
	std::vector<Operation> operations;
	std::vector<uint8_t> active_qbits;

	bool valid = true;
	std::string error_message;

public:
	Quantum_Program(std::string source_code);

	bool is_valid() const;
	std::string get_build_error() const;

	std::vector<Operation> const &get_operations() const;
	std::vector<uint8_t> const &get_active_qbits() const;

private:
	void add_operation(Gate gate, struct Expression const &expression, uint8_t num_operands, bool has_immediate = false);
	void set_error(uint32_t line_number, std::string const &error);
};
