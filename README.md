# Manipulates-logical-expression-trees
Project Description
The goal of this project is to design an application that manipulates logical expression trees with variables. The
program will convert infix expressions to postfix notation, construct a binary expression tree, simplify it using logical
identities, and evaluate it.
Main Functionalities
The application should implement the following core features:
• Convert an infix logical expression to postfix notation using a stack.
• Build a binary expression tree from the postfix expression, supporting:– Logical operators: AND, OR, NOT.– Relational operators: <, >, <=, >=, ==, !=
• Simplify the expression tree, including:– Constant folding (e.g., true AND false → false, 5 < 3 → false)– Removing neutral elements (e.g., x AND true → x, x OR true → true)– Simplifying unary logical operators where applicable (e.g., NOT(false) → true, NOT(NOT(true)) → true)
• Evaluate the expression tree to produce a Boolean result.
Error Handling
Your program must include robust input validation and error handling:
• Detect unbalanced parentheses.
• Reject malformed expressions.
• Handle unexpected tokens or characters.


