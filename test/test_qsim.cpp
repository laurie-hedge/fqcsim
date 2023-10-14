#include <algorithm>

#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"
#include "constants.h"
#include "qasm.h"
#include "qsim.h"

using namespace std::literals;

struct QSim_Test_Fixture
{
	Quantum_Program program;
	QSim sim;
	std::vector<Amplitude> amplitudes;

	QSim_Test_Fixture(std::string const &source)
		: program(source)
	{
		sim.set_program(&program);
		sim.run(1);
		amplitudes = sim.get_amplitudes();
	}

	bool has_state_amplitude(uint8_t state, std::complex<double> amplitude) const
	{
		auto state_amplitude = std::find_if(amplitudes.begin(), amplitudes.end(),
	                                        [state](Amplitude const &amplitude)
	                                        { return amplitude.state == state; });
		if (state_amplitude == amplitudes.end()) {
			return false;
		}
		return std::abs(state_amplitude->amplitude - amplitude) < 0.001;
	}
};

TEMPLATE_TEST_CASE_SIG("QSim CNot Gate", "[qsim]",
                       ((int program_index, uint8_t state), program_index, state),
                       (0, 0b00000000), (1, 0b01000000), (2, 0b11000000), (3, 0b10000000))
{
	static std::string programs[] = {
		"cnot q0 q1",
		"x q1\ncnot q0 q1",
		"x q0\ncnot q0 q1",
		"x q0\nx q1\ncnot q0 q1",
	};
	QSim_Test_Fixture fixture(programs[program_index]);
	REQUIRE(fixture.has_state_amplitude(state, 1.0));
}

TEST_CASE("QSim CNot Gate Across Other Qbits", "[qsim]")
{
	QSim_Test_Fixture fixture { "x q1\nh q2\nx q3\ncnot q1 q4" };
	REQUIRE(fixture.has_state_amplitude(0b01011000, 1.0 / std::sqrt(2.0)));
	REQUIRE(fixture.has_state_amplitude(0b01111000, 1.0 / std::sqrt(2.0)));
}

TEST_CASE("QSim Identity Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "i q0" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, 1.0));
}

TEST_CASE("QSim Hadamard Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "h q0" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, 1.0 / std::sqrt(2.0)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, 1.0 / std::sqrt(2.0)));
}

TEST_CASE("QSim Pauli X Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "x q1" };
	REQUIRE(fixture.has_state_amplitude(0b01000000, 1.0));
}

TEST_CASE("QSim Pauli Y Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "y q2" };
	REQUIRE(fixture.has_state_amplitude(0b00100000, 1.0i));
}

TEST_CASE("QSim Pauli Z Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "h q3\nz q3" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, 1.0 / std::sqrt(2.0)));
	REQUIRE(fixture.has_state_amplitude(0b00010000, -1.0 / std::sqrt(2.0)));
}

TEST_CASE("QSim Rx Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "rx q0 1" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, std::cos(0.5)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, -1i * std::sin(0.5)));
}

TEST_CASE("QSim Ry Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "ry q0 1" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, std::cos(0.5)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, -std::sin(0.5)));
}

TEST_CASE("QSim Rz Gate", "[qsim]")
{
	double const e = std::exp(1.0);
	QSim_Test_Fixture fixture { "h q0\nrz q0 1" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, (1.0 / std::sqrt(2.0)) * std::pow(e, -0.5i)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, (1.0 / std::sqrt(2.0)) * std::pow(e, 0.5i)));
}

TEST_CASE("QSim S Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "h q0\ns q0" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, 1.0 / std::sqrt(2.0)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, 1i * (1.0 / std::sqrt(2.0))));
}

TEST_CASE("QSim Sdag Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "h q0\nsdag q0" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, 1.0 / std::sqrt(2.0)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, -1i * (1.0 / std::sqrt(2.0))));
}

TEST_CASE("QSim Swap Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "x q0\nswap q0 q1" };
	REQUIRE(fixture.has_state_amplitude(0b01000000, 1.0));
}

TEST_CASE("QSim T Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "h q0\nt q0" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, 1.0 / std::sqrt(2.0)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, std::exp((1i * CONST_PI) / 4.0) * (1.0 / std::sqrt(2.0))));
}

TEST_CASE("QSim Tdag Gate", "[qsim]")
{
	QSim_Test_Fixture fixture { "h q0\ntdag q0" };
	REQUIRE(fixture.has_state_amplitude(0b00000000, 1.0 / std::sqrt(2.0)));
	REQUIRE(fixture.has_state_amplitude(0b10000000, std::exp((-1i * CONST_PI) / 4.0) * (1.0 / std::sqrt(2.0))));
}

TEMPLATE_TEST_CASE_SIG("QSim Toffoli Gate", "[qsim]",
                       ((int program_index, uint8_t state), program_index, state),
                       (0, 0b00000000), (1, 0b00100000), (2, 0b01000000), (3, 0b01100000), (4, 0b10000000),
                       (5, 0b10100000), (6, 0b11100000), (7, 0b11000000))
{
	static std::string const programs[] = {
		"toffoli q0 q1 q2",
		"x q2\ntoffoli q0 q1 q2",
		"x q1\ntoffoli q0 q1 q2",
		"x q1\nx q2\ntoffoli q0 q1 q2",
		"x q0\ntoffoli q0 q1 q2",
		"x q0\nx q2\ntoffoli q0 q1 q2",
		"x q0\nx q1\ntoffoli q0 q1 q2",
		"x q0\nx q1\nx q2\ntoffoli q0 q1 q2",
	};
	QSim_Test_Fixture fixture(programs[program_index]);
	REQUIRE(fixture.has_state_amplitude(state, 1.0));
}
