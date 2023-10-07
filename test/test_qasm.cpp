#include <algorithm>
#include <string>
#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "qasm.h"

TEST_CASE("Valid Qasm Program Compiles All Gates", "[qasm]")
{
	std::string source = "CNot q1 q2\n"
	                     "I Q0\n"
	                     "h q7\n"
	                     "X q3 \n"
	                     "\ty\tq4\t\n"
	                     "z q4 # line comment\n"
	                     "\n"
	                     "# comment\n"
	                     "rX\t \tq5 1.5\n"
	                     " Ry q6\t2.0\n"
	                     "rz Q6 2.5 #line comment\n"
	                     "s q0\n"
	                     "sdag q1\n"
	                     "swap q2\tq3\n"
	                     "t q4\n"
	                     "tdag q5   \n"
	                     "toffoli q6 q7 q0\n"
	                     ;
	Quantum_Program program(source);

	// check program compiles
	REQUIRE(program.is_valid());

	// check all qbits are active
	std::vector<uint8_t> const &active_qbits = program.get_active_qbits();
	for (uint8_t qbit_index = 0; qbit_index < 8; ++qbit_index) {
		REQUIRE(std::find(active_qbits.begin(), active_qbits.end(), qbit_index) != active_qbits.end());
	}

	// check gates decoded correctly
	std::vector<Operation> const &operations = program.get_operations();
	REQUIRE(operations[0].gate == Gate::CNOT);
	REQUIRE(operations[0].operands[0] == 1);
	REQUIRE(operations[0].operands[1] == 2);

	REQUIRE(operations[1].gate == Gate::IDENTITY);
	REQUIRE(operations[1].operands[0] == 0);

	REQUIRE(operations[2].gate == Gate::HADAMARD);
	REQUIRE(operations[2].operands[0] == 7);

	REQUIRE(operations[3].gate == Gate::PAULI_X);
	REQUIRE(operations[3].operands[0] == 3);

	REQUIRE(operations[4].gate == Gate::PAULI_Y);
	REQUIRE(operations[4].operands[0] == 4);

	REQUIRE(operations[5].gate == Gate::PAULI_Z);
	REQUIRE(operations[5].operands[0] == 4);

	REQUIRE(operations[6].gate == Gate::R_X);
	REQUIRE(operations[6].operands[0] == 5);
	REQUIRE(operations[6].immediate == 1.5);

	REQUIRE(operations[7].gate == Gate::R_Y);
	REQUIRE(operations[7].operands[0] == 6);
	REQUIRE(operations[7].immediate == 2.0);

	REQUIRE(operations[8].gate == Gate::R_Z);
	REQUIRE(operations[8].operands[0] == 6);
	REQUIRE(operations[8].immediate == 2.5);

	REQUIRE(operations[9].gate == Gate::S);
	REQUIRE(operations[9].operands[0] == 0);

	REQUIRE(operations[10].gate == Gate::S_DAG);
	REQUIRE(operations[10].operands[0] == 1);

	REQUIRE(operations[11].gate == Gate::SWAP);
	REQUIRE(operations[11].operands[0] == 2);
	REQUIRE(operations[11].operands[1] == 3);

	REQUIRE(operations[12].gate == Gate::T);
	REQUIRE(operations[12].operands[0] == 4);

	REQUIRE(operations[13].gate == Gate::T_DAG);
	REQUIRE(operations[13].operands[0] == 5);

	REQUIRE(operations[14].gate == Gate::TOFFOLI);
	REQUIRE(operations[14].operands[0] == 6);
	REQUIRE(operations[14].operands[1] == 7);
	REQUIRE(operations[14].operands[2] == 0);
}

TEST_CASE("Qasm Reports Correct Error Line Number", "[qasm]")
{
	std::string source = "i q0\ni q0\npudding\ni q0\n";
	Quantum_Program program(source);

	// check program fails to compile
	REQUIRE(!program.is_valid());

	// check error is reported on line 3
	REQUIRE(program.get_build_error().find("line 3:") != std::string::npos);
}

TEST_CASE("Qasm Correctly Reports Active QBits", "[qasm]")
{
	std::string source = "i q1\ni q3\ni q6";
	Quantum_Program program(source);

	// check program compiles
	REQUIRE(program.is_valid());

	// check active qbits reported correctly, in order from MSB to LSB
	std::vector<uint8_t> const &active_qbits = program.get_active_qbits();
	REQUIRE(active_qbits.size() == 3);
	REQUIRE(active_qbits[0] == 6);
	REQUIRE(active_qbits[1] == 3);
	REQUIRE(active_qbits[2] == 1);
}

TEST_CASE("Qasm Rejects Invalid Commands", "[qasm]")
{
	std::vector<std::string> commands = {
		"i q9\n",                // invalid qbit index
		"abc q0\n",              // invalid gate
		"i x0\n",                // invalid qbit
		"i\n",                   // missing argument
		"i q0 q1\n",             // too many arguments
		"swap q0\n",             // missing argument
		"swap q0 q1 q2\n",       // too many arguments
		"swap q3 q3\n",          // duplicated argument
		"rx q0 q1\n",            // second argument qbit instead of immediate
		"ry 1.0 2.0\n",          // first argument immediate instead of qbit
		"rz q4 0.1abc\n",        // immediate not numeric
		"x 1.0\n",               // immediate argument instead of qbit
		"toffoli q0 q1\n",       // missing argument
		"toffoli q0 q0 q1\n",    // duplicated argument
		"toffoli q0 q1 q0\n",    // duplicated argument
		"toffoli q0 q1 q1\n",    // duplicated argument
		"toffoli q0 q0 q0\n",    // duplicated argument
		"toffoli q0 q1 q2 q3\n", // too many arguments
		"z #q0\n",               // argument commented out
	};

	for (auto const &command : commands) {
		Quantum_Program program(command);
		REQUIRE(!program.is_valid());
	}
}
