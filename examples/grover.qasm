# Source: https://www.quantum-inspire.com/kbase/grover-algorithm/

# Grover's algorithm for searching the decimal number 6 in a database of size 2^3

# init to |+++>
h q0
h q1
h q2

# oracle
x q0
h q2
toffoli q0 q1 q2
h q2
x q0

# diffusion
h q0
h q1
h q2
x q1
x q0
x q2
h q2
toffoli q0 q1 q2
h q2
x q1
x q0
x q2
h q0
h q1
h q2
