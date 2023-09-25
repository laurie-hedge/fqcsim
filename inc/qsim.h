#pragma once

#include <complex>
#include <random>
#include <vector>

class Quantum_Program;

struct Amplitude
{
	uint32_t state;
	std::complex<double> amplitude;
};

struct Result
{
	uint32_t state;
	uint32_t num_times;
};

template <typename Type>
using matrix = std::vector<std::vector<Type>>;

class QSim
{
	std::mt19937 rng;
	std::uniform_real_distribution<double> random_distribution;

	Quantum_Program const *program;
	size_t next_gate_index = 0;

	std::vector<std::complex<double>> state_vector;
	
	std::vector<Result> results;

public:
	QSim();
	~QSim();

	void set_program(Quantum_Program const *new_program);

	void reset();
	void run(int num_runs);
	void step(bool is_single_step = true);

	std::vector<Amplitude> get_amplitudes() const;
	std::vector<Result> const &get_results() const { return results; }
	size_t get_next_gate_index() const { return next_gate_index; }
	std::array<std::complex<double>, 2> get_qbit_state(uint8_t qbit) const;

private:
	void perform_quantum_gate(matrix<std::complex<double>> const &gate, uint8_t qbit);
	void perform_cnot_gate(uint8_t control_qbit, uint8_t target_qbit);
	void generate_results(int num_runs);
};
