env = Environment(CPPPATH=['inc'])

env.Append(CCFLAGS=['-g', '-Wextra', '-Wall', '-O2'])

env.Program(target=['crawler'],
            source=['main.c', Glob('core/*.c')])