from sympy import symbols, Eq, solve
x, y = symbols('x y')


equation1 = Eq(x + y , 35)
equation2 = Eq(2*x + 4*y , 94)


solution = solve((equation1 , equation2 ) , (x ,y))

print(solution)