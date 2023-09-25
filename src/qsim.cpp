#include <algorithm>
#include <array>
#include <random>

#include "constants.h"
#include "qasm.h"
#include "qsim.h"

using namespace std::literals;

static matrix<std::complex<double>> const identity = {
	{ 1.0, 0.0 },
	{ 0.0, 1.0 }
};

static matrix<std::complex<double>> const hadamard = {
	{ 1.0 / std::sqrt(2.0), 1.0 / std::sqrt(2.0) },
	{ 1.0 / std::sqrt(2.0), -1.0 / std::sqrt(2.0) }
};

static matrix<std::complex<double>> const pauli_x = {
	{ 0.0, 1.0 },
	{ 1.0, 0.0 }
};

static matrix<std::complex<double>> const pauli_y = {
	{ 0.0, -1.0i },
	{ 1.0i, 0.0 }
};

static matrix<std::complex<double>> const pauli_z = {
	{ 1.0, 0.0 },
	{ 0.0, -1.0 }
};

static matrix<std::complex<double>> const s_gate = {
	{ 1.0, 0.0 },
	{ 0.0, 1.0i }
};

static matrix<std::complex<double>> const s_dag_gate = {
	{ 1.0, 0.0 },
	{ 0.0, -1.0i }
};

static matrix<std::complex<double>> const t_gate = {
	{ 1.0, 0.0 },
	{ 0.0, std::exp((1.0i * CONST_PI) / 4.0) }
};

static matrix<std::complex<double>> const t_dag_gate = {
	{ 1.0, 0.0 },
	{ 0.0, std::exp((-1.0i * CONST_PI) / 4.0) }
};

static matrix<std::complex<double>> const zero_projector = {
	{ 1.0, 0.0 },
	{ 0.0, 0.0 }
};

static matrix<std::complex<double>> const one_projector = {
	{ 0.0, 0.0 },
	{ 0.0, 1.0 }
};

static matrix<std::complex<double>> build_rx(double theta)
{
	return {
		{ std::cos(theta / 2.0), -1.0i * std::sin(theta / 2.0) },
		{ -1.0i * std::sin(theta / 2.0), std::cos(theta / 2.0) }
	};
}

static matrix<std::complex<double>> build_ry(double theta)
{
	return {
		{ std::cos(theta / 2.0), -std::sin(theta / 2.0) },
		{ -std::sin(theta / 2.0), std::cos(theta / 2.0) }
	};
}

static matrix<std::complex<double>> build_rz(double theta)
{
	double const e = std::exp(1.0);
	return {
		{ std::pow(e, -1.0i * (theta / 2.0)), 0.0 },
		{ 0.0, std::pow(e, 1.0i * (theta / 2.0)) }
	};
}

template <typename Type>
static matrix<Type> tensor_product(matrix<Type> const &lhs, matrix<Type> const &rhs)
{
	size_t const lhs_width = lhs[0].size();
	size_t const lhs_height = lhs.size();
	size_t const rhs_width = rhs[0].size();
	size_t const rhs_height = rhs.size();
	size_t const result_width = lhs_width * rhs_width;
	size_t const result_height = lhs_height * rhs_height;

	matrix<Type> result;
	result.resize(result_height);
	for (auto &row : result) {
		row.resize(result_width);
	}

	for (size_t y = 0; y < result_height; ++y) {
		for (size_t x = 0; x < result_width; ++x) {
			size_t const lhs_x = x / rhs_width;
			size_t const lhs_y = y / rhs_height;
			size_t const rhs_x = x % rhs_width;
			size_t const rhs_y = y % rhs_height;
			result[y][x] = lhs[lhs_y][lhs_x] * rhs[rhs_y][rhs_x];
		}
	}

	return result;
}

template <typename Type>
static std::vector<Type> vec_mat_mul(std::vector<Type> const &vec, matrix<Type> const &mat)
{
	size_t const size = vec.size();

	std::vector<Type> result;
	result.resize(size);

	for (size_t y = 0; y < size; ++y) {
		result[y] = 0.0;
		for (size_t x = 0; x < size; ++x) {
			result[y] += vec[x] * mat[y][x];
		}
	}
	return result;
}

template <typename Type>
static matrix<Type> mat_mat_add(matrix<Type> const &lhs, matrix<Type> const &rhs)
{
	size_t const width = lhs[0].size();
	size_t const height = lhs.size();

	matrix<Type> result;
	result.resize(height);
	for (auto &row : result) {
		row.resize(width);
	}

	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			result[y][x] = lhs[y][x] + rhs[y][x];
		}
	}

	return result;
}

template <typename Type>
static Type dot_product(std::vector<Type> const &lhs, std::vector<Type> const &rhs)
{
	Type result = 0.0;
	for (size_t index = 0; index < lhs.size(); ++index) {
		result += lhs[index] * rhs[index];
	}
	return result;
}

QSim::QSim() :
	random_distribution(0.0, 1.0)
{
	reset();
}

QSim::~QSim()
{
}

void QSim::set_program(Quantum_Program const *new_program)
{
	program = new_program;
	reset();
}

void QSim::reset()
{
	next_gate_index = 0;
	state_vector.resize(STATE_VEC_SIZE);
	std::fill(state_vector.begin(), state_vector.end(), 0.0);
	state_vector[0] = 1.0;
}

void QSim::run(int num_runs)
{
	reset();
	if (program) {
		while (next_gate_index < program->get_operations().size()) {
			step(false);
		}
	}

	generate_results(num_runs);
}

void QSim::step(bool is_single_step)
{
	if (program && next_gate_index < program->get_operations().size()) {
		Operation const &operation = program->get_operations()[next_gate_index];
		switch (operation.gate) {
			case Gate::CNOT: {
				perform_cnot_gate(operation.operands[0], operation.operands[1]);
			} break;
			case Gate::IDENTITY: {
				perform_quantum_gate(identity, operation.operands[0]);
			} break;
			case Gate::HADAMARD: {
				perform_quantum_gate(hadamard, operation.operands[0]);
			} break;
			case Gate::PAULI_X: {
				perform_quantum_gate(pauli_x, operation.operands[0]);
			} break;
			case Gate::PAULI_Y: {
				perform_quantum_gate(pauli_y, operation.operands[0]);
			} break;
			case Gate::PAULI_Z: {
				perform_quantum_gate(pauli_z, operation.operands[0]);
			} break;
			case Gate::R_X: {
				matrix<std::complex<double>> const r_x = build_rx(operation.immediate);
				perform_quantum_gate(r_x, operation.operands[0]);
			} break;
			case Gate::R_Y: {
				matrix<std::complex<double>> const r_y = build_ry(operation.immediate);
				perform_quantum_gate(r_y, operation.operands[0]);
			} break;
			case Gate::R_Z: {
				matrix<std::complex<double>> const r_z = build_rz(operation.immediate);
				perform_quantum_gate(r_z, operation.operands[0]);
			} break;
			case Gate::S: {
				perform_quantum_gate(s_gate, operation.operands[0]);
			} break;
			case Gate::S_DAG: {
				perform_quantum_gate(s_dag_gate, operation.operands[0]);
			} break;
			case Gate::SWAP: {
				perform_cnot_gate(operation.operands[0], operation.operands[1]);
				perform_cnot_gate(operation.operands[1], operation.operands[0]);
				perform_cnot_gate(operation.operands[0], operation.operands[1]);
			} break;
			case Gate::T: {
				perform_quantum_gate(t_gate, operation.operands[0]);
			} break;
			case Gate::T_DAG: {
				perform_quantum_gate(t_dag_gate, operation.operands[0]);
			} break;
			case Gate::TOFFOLI: {
				perform_quantum_gate(hadamard, operation.operands[2]);
				perform_cnot_gate(operation.operands[1], operation.operands[2]);
				perform_quantum_gate(t_dag_gate, operation.operands[2]);
				perform_cnot_gate(operation.operands[0], operation.operands[2]);
				perform_quantum_gate(t_gate, operation.operands[2]);
				perform_cnot_gate(operation.operands[1], operation.operands[2]);
				perform_quantum_gate(t_dag_gate, operation.operands[2]);
				perform_cnot_gate(operation.operands[0], operation.operands[2]);
				perform_quantum_gate(t_gate, operation.operands[1]);
				perform_quantum_gate(t_gate, operation.operands[2]);
				perform_cnot_gate(operation.operands[0], operation.operands[1]);
				perform_quantum_gate(hadamard, operation.operands[2]);
				perform_quantum_gate(t_gate, operation.operands[0]);
				perform_quantum_gate(t_dag_gate, operation.operands[1]);
				perform_cnot_gate(operation.operands[0], operation.operands[1]);
			} break;
		}
		next_gate_index += 1;

		if (is_single_step && next_gate_index == program->get_operations().size()) {
			generate_results(1);
		}
	}
}

std::vector<Amplitude> QSim::get_amplitudes() const
{
	std::vector<Amplitude> amplitudes;
	for (size_t index = 0; index < state_vector.size(); ++index) {
		if (std::abs(state_vector[index]) != 0.0) {
			amplitudes.push_back({(uint8_t)index, state_vector[index]});
		}
	}
	return amplitudes;
}

std::array<std::complex<double>, 2> QSim::get_qbit_state(uint8_t qbit) const
{
	std::complex<double> zero_probability = 0.0f;
	std::complex<double> one_probability = 0.0f;
	for (size_t state = 0; state < state_vector.size(); ++state) {
		if ((state >> (7 - qbit)) & 1) {
			one_probability += std::pow(state_vector[state], 2);
		} else {
			zero_probability += std::pow(state_vector[state], 2);
		}
	}

	return { std::sqrt(zero_probability), std::sqrt(one_probability) };
}

void QSim::perform_quantum_gate(matrix<std::complex<double>> const &gate, uint8_t qbit)
{
	std::vector<matrix<std::complex<double>>> matrices;
	for (uint8_t index = 0; index < NUM_QBITS; ++index) {
		matrices.push_back(index == qbit ? gate : identity);
	}

	matrix<std::complex<double>> mat = matrices[NUM_QBITS - 1];
	for (int index = (NUM_QBITS - 2); index >= 0; --index) {
		mat = tensor_product(matrices[index], mat);
	}

	state_vector = vec_mat_mul(state_vector, mat);
}

void QSim::perform_cnot_gate(uint8_t control_qbit, uint8_t target_qbit)
{
	// Sources
	// https://quantumcomputing.stackexchange.com/questions/4252/how-to-derive-the-cnot-matrix-for-a-3-qubit-system-where-the-control-target-qu
	// https://quantumcomputing.stackexchange.com/questions/9614/how-to-interpret-a-4-qubit-quantum-circuit-as-a-matrix

	uint8_t const high_qbit = std::max(control_qbit, target_qbit);
	uint8_t const low_qbit = std::min(control_qbit, target_qbit);
	uint8_t const num_uninvolved = high_qbit - low_qbit - 1;

	matrix<std::complex<double>> lhs, rhs;
	if (control_qbit < target_qbit) {
		lhs = identity;
		rhs = pauli_x;
		for (uint8_t i = 0; i < num_uninvolved; ++i) {
			lhs = tensor_product(identity, lhs);
			rhs = tensor_product(identity, rhs);
		}
		lhs = tensor_product(zero_projector, lhs);
		rhs = tensor_product(one_projector, rhs);
	} else {
		lhs = zero_projector;
		rhs = one_projector;
		for (uint8_t i = 0; i < num_uninvolved; ++i) {
			lhs = tensor_product(identity, lhs);
			rhs = tensor_product(identity, rhs);
		}
		lhs = tensor_product(identity, lhs);
		rhs = tensor_product(pauli_x, rhs);
	}
	matrix<std::complex<double>> const cnot = mat_mat_add(lhs, rhs);

	matrix<std::complex<double>> mat;
	if (high_qbit == (NUM_QBITS - 1)) {
		mat = cnot;
	} else {
		mat = identity;
		for (uint8_t i = (NUM_QBITS - 2); i > high_qbit; --i) {
			mat = tensor_product(mat, identity);
		}
		mat = tensor_product(cnot, mat);
	}

	for (int i = low_qbit - 1; i >= 0; --i) {
		mat = tensor_product(identity, mat);
	}

	state_vector = vec_mat_mul(state_vector, mat);
}

void QSim::generate_results(int num_runs)
{
	struct Result_Range {
		double start, end;
		uint8_t state;
		uint32_t count;
	};

	std::vector<Result_Range> ranges;
	double last_end = 0.0;
	for (size_t index = 0; index < state_vector.size(); ++index) {
		ranges.push_back({ last_end, last_end + std::abs(std::pow(state_vector[index], 2)), (uint8_t)index, 0 });
		last_end = ranges.back().end;
	}

	for (int i = 0; i < num_runs; ++i) {
		double const random_number = random_distribution(rng);
		auto selected_range = std::find_if(ranges.begin(), ranges.end(), [&](Result_Range const &range) {
			return range.start <= random_number && random_number <= range.end;
		});
		selected_range->count += 1;
	}

	results.clear();
	for (auto const &range : ranges) {
		if (range.count > 0) {
			results.push_back({ range.state, range.count });
		}
	}
}
