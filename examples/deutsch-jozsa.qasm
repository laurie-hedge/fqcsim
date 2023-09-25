# Source: https://www.quantum-inspire.com/kbase/deutsch-jozsa-algorithm/

# The Deutsch-Josza algorithm determines if a binary function is constant or balanced
# This algorithm only requires a single query of the oracle
# Constant: q0 = |0>, Balanced: q0 = |1>

# init to |+->
h q0
x q1
h q1
 
# f(x) = 0
i q1
 
# f(x) = 1
# x q1
 
# f(x) = x
# cnot q0 q1
 
# f(x) = !x
# cnot q0 q1
# x q1

# result
h q0
